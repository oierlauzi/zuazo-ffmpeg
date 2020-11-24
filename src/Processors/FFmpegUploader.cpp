#include <zuazo/Processors/FFmpegUploader.h>

#include "../FFmpeg/SWScaleContext.h"

#include <zuazo/Utils/Functions.h>
#include <zuazo/Math/Comparisons.h>
#include <zuazo/Utils/Pool.h>
#include <zuazo/Signal/Input.h>
#include <zuazo/Signal/Output.h>
#include <zuazo/Graphics/Uploader.h>
#include <zuazo/FFmpeg/Frame.h>
#include <zuazo/FFmpeg/Signals.h>
#include <zuazo/FFmpeg/FFmpegConversions.h>

#include <memory>
#include <cassert>
#include <tuple>

extern "C" {
	#include <libavutil/imgutils.h>
	#include <libavutil/pixdesc.h>
	#include <libavutil/mastering_display_metadata.h>
}

#include <iostream>

namespace Zuazo::Processors {

/*
 * FFmpegUploaderImpl
 */

struct FFmpegUploaderImpl {
	struct Open {
		Graphics::Uploader		uploader;
		FFmpeg::PixelFormat		dstPixelFormat;
		FFmpeg::SWScaleContext	swscaleContext;


		Open(	const Graphics::Vulkan& vulkan, 
				const Graphics::Frame::Descriptor& frameDesc, 
				const Chromaticities& chromaticities) 
			: uploader(vulkan, frameDesc, chromaticities)
			, dstPixelFormat(getPixelFormat(frameDesc))
			, swscaleContext()
		{
		}

		~Open() = default;

		Zuazo::Video process(const FFmpeg::Frame& frame) {
			auto result = uploader.acquireFrame();
			assert(result);
			assert(frame.getResolution() == result->getDescriptor().resolution);
			assert(dstPixelFormat == getPixelFormat(result->getDescriptor()));

			//Gather information about the destination frame
			const auto resolution = frame.getResolution();

			//Ensure that the scaler (converter) is properly set-up
			swscaleContext.recreate(
				resolution, frame.getPixelFormat(),
				resolution, dstPixelFormat,
				0x10
			);

			//Obtain the plane pointers for the source
			const auto srcBuffers = frame.getData();
			const auto srcLinesizes = frame.getLineSizes();

			//Obtain the plane pointers for the destination
			const auto& dstPixelData = result->getPixelData();
			constexpr size_t MAX_LEN = 8;
			assert(dstPixelData.size() <= MAX_LEN);
			std::array<std::byte*, MAX_LEN> dstBuffers;
			std::array<int, MAX_LEN> dstLinesizes;

			for(size_t i = 0; i < dstPixelData.size(); ++i) {
				dstBuffers[i] = dstPixelData[i].data();
				dstLinesizes[i] = av_image_get_linesize( 
					static_cast<AVPixelFormat>(dstPixelFormat),
					resolution.width,
					i
				);
			}

			//Copy the data to the destination frame
			swscaleContext.scale(
				srcBuffers.data(),
				srcLinesizes.data(),
				0, resolution.height,
				dstBuffers.data(),
				dstLinesizes.data()
			);

			result->flush();
			return result;
		}

		void recreate(const Graphics::Frame::Descriptor& frameDesc, const Chromaticities& chromaticities) {
			uploader = Graphics::Uploader(
				uploader.getVulkan(), 
				frameDesc,
				chromaticities
			);
			dstPixelFormat = getPixelFormat(frameDesc);
		}

	private:
		static FFmpeg::PixelFormat getPixelFormat(const Graphics::Frame::Descriptor& frameDesc) {
			return FFmpeg::toFFmpeg(FFmpeg::PixelFormatConversion{
				frameDesc.colorFormat,
				frameDesc.colorSubsampling,
				isYCbCr(frameDesc.colorModel)
			});
		}
	};

	using Input = Signal::Input<FFmpeg::FrameStream>;
	using Output = Signal::Output<Zuazo::Video>;

	Input 									frameIn;
	Output									videoOut;

	std::reference_wrapper<FFmpegUploader> 	owner;


	std::unique_ptr<Open> 					opened;

	FFmpegUploaderImpl(FFmpegUploader& uploader)
		: videoOut(std::string(Signal::makeOutputName<Video>()), createPullCallback(uploader))
		, owner(uploader)
	{
	}

	~FFmpegUploaderImpl() = default;

	void moved(ZuazoBase& base) {
		owner = static_cast<FFmpegUploader&>(base);
		videoOut.setPullCallback(createPullCallback(owner));
	}

	void open(ZuazoBase& base) {
		assert(!opened);
		const auto& uploader = static_cast<FFmpegUploader&>(base);
		assert(&owner.get() == &uploader); (void)(uploader);
	}

	void close(ZuazoBase& base) {
		auto& uploader = static_cast<FFmpegUploader&>(base);
		assert(&uploader == &owner.get()); (void)(uploader);

		opened.reset();
		frameIn.reset();
		videoOut.reset();
	}

	void update() {
		auto& uploader = owner.get();

		if(uploader.isOpen() && frameIn.hasChanged()) {
			const auto oldFrame = frameIn.getLastElement();
			const auto& newFrame = frameIn.pull();

			if(oldFrame && newFrame) {
				//Frame remains valid. Check if characteristics have changed
				if(	oldFrame->getResolution() != newFrame->getResolution() ||
					oldFrame->getPixelAspectRatio() != newFrame->getPixelAspectRatio() ||
					oldFrame->getColorPrimaries() != newFrame->getColorPrimaries() ||
					oldFrame->getColorSpace() != newFrame->getColorSpace() ||
					oldFrame->getColorTransferCharacteristic() != newFrame->getColorTransferCharacteristic() ||
					oldFrame->getColorRange() != newFrame->getColorRange() ||
					oldFrame->getPixelFormat() != newFrame->getPixelFormat() )
				{
					assert(newFrame);
					uploader.setVideoModeCompatibility(createVideoModeCompatibility(*newFrame)); //May call videoModeCallback() in order to recreate
				} /*else if () { //TODO evaluate if chromaticities have changed
					const auto& videoMode = uploader.getVideoMode();
					videoModeCallback(uploader, videoMode); //This will recreate
				}*/
			} else if(oldFrame && !newFrame) {
				//Frame has become invalid
				uploader.setVideoModeCompatibility({});
			} else if(!oldFrame && newFrame) {
				//Frame has become valid
				assert(newFrame);
				uploader.setVideoModeCompatibility(createVideoModeCompatibility(*newFrame));
			}

			//Convert the frame if possible
			if(opened){
				assert(newFrame); //Opened should have been reset if invalid
				videoOut.push(opened->process(*newFrame));
			} 
		}
	}

	void videoModeCallback(VideoBase& base, const VideoMode& videoMode) {
		auto& uploader = static_cast<FFmpegUploader&>(base);
		assert(&uploader == &owner.get());

		if(uploader.isOpen()) {
			const auto isValid = static_cast<bool>(videoMode);

			if(opened && isValid) {
				//It remais valid but it has changed
				assert(frameIn.getLastElement());
				const auto frameDesc = videoMode.getFrameDescriptor();

				opened->recreate(
					frameDesc,
					calculateColorPrimaries(frameDesc, *frameIn.getLastElement())
				);
			} else if (opened && !isValid) {
				//It has become invalid
				opened.reset();
				videoOut.reset();
			} else if(!opened && isValid) {
				//It has become valid
				assert(frameIn.getLastElement());
				const auto frameDesc = videoMode.getFrameDescriptor();

				opened = Utils::makeUnique<Open>(
					uploader.getInstance().getVulkan(),
					frameDesc,
					calculateColorPrimaries(frameDesc, *frameIn.getLastElement())
				);
			}
		}
	}
	
	static bool isSupportedInput(FFmpeg::PixelFormat fmt) {
		return FFmpeg::SWScaleContext::isSupportedInput(fmt);
	}

private:
	std::vector<VideoMode> createVideoModeCompatibility(const FFmpeg::Frame& frame) const {	
		const auto& uploader = owner.get();
		std::vector<VideoMode> result;

		const auto frameResolution = frame.getResolution();
		const auto framePixelAspectRatio = frame.getPixelAspectRatio();
		const auto frameColorPrimaries = frame.getColorPrimaries();
		const auto frameColorSpace = frame.getColorSpace();
		const auto frameColorTransferFunction = frame.getColorTransferCharacteristic();
		const auto frameColorRange = frame.getColorRange();
		const auto framePixelFormat = frame.getPixelFormat();

		const auto fmtConversion = FFmpeg::fromFFmpeg(getBestConversion(uploader.getInstance().getVulkan(), framePixelFormat));
		constexpr auto defaultPixelAspectRatio = AspectRatio(1, 1);
		const auto defaultColorPrimaries = ColorPrimaries::BT709;
		const auto defaultColorModel = fmtConversion.isYCbCr ? ColorModel::BT709 : ColorModel::RGB;
		constexpr auto defaultColorTransferFunction = ColorTransferFunction::BT709;
		const auto defaultColorRange = fmtConversion.isYCbCr ? ColorRange::ITU_NARROW : ColorRange::FULL;

		const auto resolution = frameResolution;
		const auto pixelAspectRatio = (framePixelAspectRatio != AspectRatio(0, 1)) ? framePixelAspectRatio : defaultPixelAspectRatio;
		const auto colorPrimaries = (frameColorPrimaries != FFmpeg::ColorPrimaries::NONE) ? FFmpeg::fromFFmpeg(frameColorPrimaries) : defaultColorPrimaries;
		const auto colorModel = (frameColorSpace != FFmpeg::ColorSpace::NONE) ? FFmpeg::fromFFmpeg(frameColorSpace) : defaultColorModel;
		const auto colorTransferFunction = (frameColorTransferFunction != FFmpeg::ColorTransferCharacteristic::NONE) ? FFmpeg::fromFFmpeg(frameColorTransferFunction) : defaultColorTransferFunction;
		const auto colorSubsampling = fmtConversion.colorSubsampling;
		const auto colorRange = (frameColorRange != FFmpeg::ColorRange::NONE) ? FFmpeg::fromFFmpeg(frameColorRange) : defaultColorRange;
		const auto colorFormat = fmtConversion.colorFormat;

		result.emplace_back(
			Utils::Limit<Rate>(Utils::Any<Rate>()),
			resolution ? Utils::Limit<Resolution>(Utils::MustBe<Resolution>(resolution)) : Utils::Limit<Resolution>(),
			pixelAspectRatio != AspectRatio(0, 1) ? Utils::Limit<AspectRatio>(Utils::MustBe<AspectRatio>(pixelAspectRatio)) : Utils::Limit<AspectRatio>(),
			colorPrimaries != ColorPrimaries::NONE ? Utils::Limit<ColorPrimaries>(Utils::MustBe<ColorPrimaries>(colorPrimaries)) : Utils::Limit<ColorPrimaries>(),
			colorModel != ColorModel::NONE ? Utils::Limit<ColorModel>(Utils::MustBe<ColorModel>(colorModel)) : Utils::Limit<ColorModel>(),
			colorTransferFunction != ColorTransferFunction::NONE ? Utils::Limit<ColorTransferFunction>(Utils::MustBe<ColorTransferFunction>(colorTransferFunction)) : Utils::Limit<ColorTransferFunction>(),
			colorSubsampling != ColorSubsampling::NONE ? Utils::Limit<ColorSubsampling>(Utils::MustBe<ColorSubsampling>(colorSubsampling)) : Utils::Limit<ColorSubsampling>(),
			colorRange != ColorRange::NONE ? Utils::Limit<ColorRange>(Utils::MustBe<ColorRange>(colorRange)) : Utils::Limit<ColorRange>(),
			colorFormat != ColorFormat::NONE ? Utils::Limit<ColorFormat>(Utils::MustBe<ColorFormat>(colorFormat)) : Utils::Limit<ColorFormat>()
		);
		
		return result;
	}

	static Chromaticities calculateColorPrimaries(	const Graphics::Frame::Descriptor& frameDesc, 
													const FFmpeg::Frame& frame ) 
	{
		Chromaticities result = getChromaticities(frameDesc.colorPrimaries);

		//Evaluate if there is any information about the mastering display
		const auto sideData = frame.getSideData();
		const auto ite = std::find_if(
			sideData.cbegin(), sideData.cend(),
			[] (const FFmpeg::FrameSideData& entry) -> bool {
				return entry.getType() == FFmpeg::FrameSideDataType::MASTERING_DISPLAY_METADATA;
			}
		);
		if(ite != sideData.cend()) {
			//It contains information about the mastering display
			assert(ite->getData().size() >= sizeof(AVMasteringDisplayMetadata));
			const auto& masteringDisplayMetadata = *reinterpret_cast<const AVMasteringDisplayMetadata*>(ite->getData().data());

			if(masteringDisplayMetadata.has_luminance) {
				const auto luminance = av_q2d(masteringDisplayMetadata.max_luminance);
				result.setWhiteLuminance(luminance);
			}

			if(masteringDisplayMetadata.has_primaries) {
				const auto red_x = av_q2d(masteringDisplayMetadata.display_primaries[0][0]);
				const auto red_y = av_q2d(masteringDisplayMetadata.display_primaries[0][1]);
				const auto green_x = av_q2d(masteringDisplayMetadata.display_primaries[1][0]);
				const auto green_y = av_q2d(masteringDisplayMetadata.display_primaries[1][1]);
				const auto blue_x = av_q2d(masteringDisplayMetadata.display_primaries[2][0]);
				const auto blue_y = av_q2d(masteringDisplayMetadata.display_primaries[2][1]);
				const auto white_x = av_q2d(masteringDisplayMetadata.white_point[0]);
				const auto white_y = av_q2d(masteringDisplayMetadata.white_point[1]);

				result.setRedPrimary(Math::Vec2f(red_x, red_y));
				result.setGreenPrimary(Math::Vec2f(green_x, green_y));
				result.setBluePrimary(Math::Vec2f(blue_x, blue_y));
				result.setWhitePoint(Math::Vec2f(white_x, white_y));
			}
		}

		return result;
	}

	static Output::PullCallback createPullCallback(FFmpegUploader& uplo) {
		return [&uplo] (Output&) -> void {
			uplo.update();
		};
	}

	static FFmpeg::PixelFormat getBestConversion(	const Graphics::Vulkan& vulkan, 
													FFmpeg::PixelFormat srcFormat ) 
	{
		FFmpeg::PixelFormat best = FFmpeg::PixelFormat::NONE;
		int loss;
		const auto& compatibleFormats = Graphics::Uploader::getSupportedFormats(vulkan);

		//Obtain info about the source format
		const AVPixFmtDescriptor* pixDesc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(srcFormat));
		assert(pixDesc);
		const ColorSubsampling colorSubsampling = subsamplingFromLog2(pixDesc->log2_chroma_w, pixDesc->log2_chroma_h);
		const bool ycbcr = !(pixDesc->flags & AV_PIX_FMT_FLAG_RGB); //FIXME grayscales are interpreted as non-rgb

		for(const auto& format : compatibleFormats) {
			//Convert the parameters into a ffmpeg format
			const auto conversion = FFmpeg::toFFmpeg(FFmpeg::PixelFormatConversion{format, colorSubsampling, ycbcr});

			if(static_cast<int>(conversion) >= 0 && FFmpeg::SWScaleContext::isSupportedOutput(conversion)) {
				//This format is supported both by swscale and zuazo
				loss = 0;
				best = static_cast<FFmpeg::PixelFormat>(av_find_best_pix_fmt_of_2(
					static_cast<AVPixelFormat>(best),
					static_cast<AVPixelFormat>(conversion),
					static_cast<AVPixelFormat>(srcFormat),
					1, //Use alpha if available
					&loss
				));
			}
		} 

		return best;
	}

	static ColorSubsampling subsamplingFromLog2(uint8_t hor, uint8_t vert) {
		ColorSubsampling result;

		//Put in a single integer both values shifting them
		const uint16_t hvSubsampling = (hor << (sizeof(vert)*Utils::getByteSize())) + vert;

		//Based on that combined integer, decide the result
		switch(hvSubsampling) {
		//log2  H W
		case 0x0000: result = ColorSubsampling::RB_444; break;
		case 0x0001: result = ColorSubsampling::RB_440; break;
		case 0x0100: result = ColorSubsampling::RB_422; break;
		case 0x0101: result = ColorSubsampling::RB_420; break;
		case 0x0200: result = ColorSubsampling::RB_411; break;
		case 0x0201: result = ColorSubsampling::RB_410; break;
		default: 	 result = ColorSubsampling::NONE;   break;
		}

		//Check the result. Other subsamplings not in the list are unexpected from FFmpeg
		assert(Math::Vec2i(1<<hor, 1<<vert) == getSubsamplingFactor(result));
		return result;
	}
};



/*
 * FFmpegUploader
 */

FFmpegUploader::FFmpegUploader(Instance& instance, std::string name, VideoMode videoMode)
	: Utils::Pimpl<FFmpegUploaderImpl>({}, *this)
	, ZuazoBase(
		instance, 
		std::move(name),
		{ (*this)->frameIn, (*this)->videoOut },
		std::bind(&FFmpegUploaderImpl::moved, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegUploaderImpl::open, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegUploaderImpl::close, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegUploaderImpl::update, std::ref(**this)) )
	, VideoBase(
		std::move(videoMode),
		std::bind(&FFmpegUploaderImpl::videoModeCallback, std::ref(**this), std::placeholders::_1, std::placeholders::_2) )
	, Signal::ProcessorLayout<FFmpeg::FrameStream, Video>(makeProxy((*this)->frameIn), makeProxy((*this)->videoOut))
{
}

FFmpegUploader::FFmpegUploader(FFmpegUploader&& other) = default;

FFmpegUploader::~FFmpegUploader() = default;

FFmpegUploader& FFmpegUploader::operator=(FFmpegUploader&& other) = default;

bool FFmpegUploader::isSupportedInput(FFmpeg::PixelFormat fmt) {
	return FFmpegUploaderImpl::isSupportedInput(fmt);
}

}
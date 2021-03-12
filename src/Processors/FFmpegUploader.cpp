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
	#include <libavutil/hwcontext.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/pixdesc.h>
	#include <libavutil/mastering_display_metadata.h>
}

namespace Zuazo::Processors {

/*
 * FFmpegUploaderImpl
 */

struct FFmpegUploaderImpl {
	struct Open {
		Graphics::Uploader		uploader;
		FFmpeg::Frame			intermediateFrame;
		FFmpeg::Frame			dstFrame;
		FFmpeg::SWScaleContext	swscaleContext;


		Open(	const Graphics::Vulkan& vulkan, 
				const Graphics::Frame::Descriptor& frameDesc, 
				const Chromaticities& chromaticities) 
			: uploader(vulkan, frameDesc, chromaticities)
			, intermediateFrame()
			, dstFrame()
			, swscaleContext()
		{
			fillFrameData(dstFrame, frameDesc);

			//HACK av_hwframe_transfer_data() checks if buf[0] is set in order to not allocate data
			//Allocate an empty buffer
			static_cast<AVFrame*>(dstFrame)->buf[0] = av_buffer_create(
				nullptr, 0, //No data
				[] (void*, uint8_t*) -> void {},
				nullptr, 0
			);
		}

		~Open() = default;

		Zuazo::Video process(const FFmpeg::Frame& frame) {
			auto result = uploader.acquireFrame();
			assert(result);
			assert(frame.getResolution() == dstFrame.getResolution());

			//Gather information about the destination frame
			const auto resolution = frame.getResolution();
			auto* hwAccelBuffer = static_cast<const AVFrame*>(frame)->hw_frames_ctx;

			//Fill the data pointers in the destination frame
			for(size_t i = 0; i < dstFrame.getData().size(); ++i) {
				dstFrame.getData()[i] = (i < result->getPixelData().size()) ?
										result->getPixelData()[i].data() :
										nullptr ;
			}

			//Evaluate if the frame needs to be downloaded
			if(hwAccelBuffer) {
				//This is a hardware accelerated frame
				//Obtain which formats are supported for destination
				FFmpeg::PixelFormat *supportedFormatsBegin, *supportedFormatsEnd;
				av_hwframe_transfer_get_formats( //FIXME this function allocates data, undesired for real-time
					hwAccelBuffer,
					AV_HWFRAME_TRANSFER_DIRECTION_FROM,
					reinterpret_cast<AVPixelFormat**>(&supportedFormatsBegin),
					0
				);
				supportedFormatsEnd = supportedFormatsBegin;
				while(*supportedFormatsEnd != FFmpeg::PixelFormat::NONE) ++supportedFormatsEnd; //Advance the end pointer til the end of the array

				//Evaluate if any conversion is needed
				if(std::find(supportedFormatsBegin, supportedFormatsEnd, dstFrame.getPixelFormat()) != supportedFormatsEnd) {
					//Destination format is directly supported for download
					//Transfer the data to the destination
					av_hwframe_transfer_data(
						static_cast<AVFrame*>(dstFrame),
						static_cast<const AVFrame*>(frame),
						0
					);
				} else {
					//TODO evaluate if intermediateFrame remains compatible for download
					//Download the data. This will call reallocate the frame if necessary
					av_hwframe_transfer_data(
						static_cast<AVFrame*>(intermediateFrame),
						static_cast<const AVFrame*>(frame),
						0
					);

					//Copy the data from the intermediate frame
					convert(dstFrame, intermediateFrame);
				}

				//Format list must be freed
				av_freep(&supportedFormatsBegin);

			} else {
				//Not a hw frame. Simply copy data from the source frame
				convert(dstFrame, frame);
			}

			result->flush();
			return result;
		}

		void recreate(const Graphics::Frame::Descriptor& frameDesc, const Chromaticities& chromaticities) {
			uploader = Graphics::Uploader(
				uploader.getVulkan(), 
				frameDesc,
				chromaticities
			);
			fillFrameData(dstFrame, frameDesc);
		}

	private:
		void convert(FFmpeg::Frame& dst, const FFmpeg::Frame& src) {
			if(dst.getPixelFormat() == src.getPixelFormat()) {
				//No need for conversion, simply copy
				av_frame_copy(
					static_cast<AVFrame*>(dst), 
					static_cast<const AVFrame*>(src)
				);
			} else {
				//A conversion needs to be done
				constexpr int SWS_NO_SCALING_FILTER = 0x10;

				//Ensure that the scaler (converter) is properly set-up
				swscaleContext.recreate(
					src.getResolution(), src.getPixelFormat(),
					dst.getResolution(), dst.getPixelFormat(),
					SWS_NO_SCALING_FILTER
				);

				//Convert
				swscaleContext.scale(
					src.getData().data(),
					src.getLineSizes().data(),
					0, src.getResolution().height,
					dst.getData().data(),
					dst.getLineSizes().data()
				);
			}

		}

		static void fillFrameData(FFmpeg::Frame& frame, const Graphics::Frame::Descriptor& frameDesc) {
			const auto resolution = frameDesc.getResolution();
			const auto format = FFmpeg::toFFmpeg(FFmpeg::PixelFormatConversion{
				frameDesc.getColorFormat(),
				frameDesc.getColorSubsampling(),
				isYCbCr(frameDesc.getColorModel())
			});

			//Set the most important parameters
			frame.setResolution(resolution);
			frame.setPixelFormat(format);

			//Fill the line sizes
			av_image_fill_linesizes(
				static_cast<AVFrame*>(frame)->linesize,
				static_cast<AVPixelFormat>(format),
				resolution.width
			);
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

	void open(ZuazoBase& base, std::unique_lock<Instance>* lock = nullptr) {
		const auto& uploader = static_cast<FFmpegUploader&>(base);
		assert(&owner.get() == &uploader); (void)(uploader);
		assert(!opened);
		Utils::ignore(lock); //Just to avoid warnings
		//It gets opened with update()
	}

	void asyncOpen(ZuazoBase& base, std::unique_lock<Instance>& lock) {
		assert(lock.owns_lock());
		open(base, &lock);
		assert(lock.owns_lock());
	}

	void close(ZuazoBase& base, std::unique_lock<Instance>* lock = nullptr) {
		auto& uploader = static_cast<FFmpegUploader&>(base);
		assert(&uploader == &owner.get()); (void)(uploader);

		//Apply changes while locked
		auto oldOpened = std::move(opened);
		frameIn.reset();
		videoOut.reset();
		
		//Destroy stuff while unlocked
		if(oldOpened) {
			if(lock) lock->unlock();
			oldOpened.reset();
			if(lock) lock->lock();
		}

		assert(!opened);
	}

	void asyncClose(ZuazoBase& base, std::unique_lock<Instance>& lock) {
		assert(lock.owns_lock());
		close(base, &lock);
		assert(lock.owns_lock());
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
		return isHardwarePixelFormat(fmt) || FFmpeg::SWScaleContext::isSupportedInput(fmt);
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
		const auto framePixelFormat = getFramePixelFormat(frame);

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
		Chromaticities result = getChromaticities(frameDesc.getColorPrimaries());

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

	static FFmpeg::PixelFormat getFramePixelFormat(const FFmpeg::Frame& frame) {
		FFmpeg::PixelFormat result;
		auto* hwCtx = static_cast<const AVFrame*>(frame)->hw_frames_ctx;

		if(hwCtx) {
			FFmpeg::PixelFormat *list = nullptr;
			av_hwframe_transfer_get_formats(
				hwCtx,
				AV_HWFRAME_TRANSFER_DIRECTION_FROM,
				reinterpret_cast<AVPixelFormat**>(&list),
				0
			);

			assert(list);
			result = list[0];
		} else {
			result = frame.getPixelFormat();
		}

		return result;
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
		const ColorSubsampling colorSubsampling = FFmpeg::subsamplingFromLog2(pixDesc->log2_chroma_w, pixDesc->log2_chroma_h);
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
		std::bind(&FFmpegUploaderImpl::open, std::ref(**this), std::placeholders::_1, nullptr),
		std::bind(&FFmpegUploaderImpl::asyncOpen, std::ref(**this), std::placeholders::_1, std::placeholders::_2),
		std::bind(&FFmpegUploaderImpl::close, std::ref(**this), std::placeholders::_1, nullptr),
		std::bind(&FFmpegUploaderImpl::asyncClose, std::ref(**this), std::placeholders::_1, std::placeholders::_2),
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
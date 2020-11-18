#include <zuazo/Processors/FFmpegUploader.h>

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
}

#include <iostream>

namespace Zuazo::Processors {

/*
 * FFmpegUploaderImpl
 */

struct FFmpegUploaderImpl {
	struct Open {
		Graphics::Uploader		uploader;

		Open(const Graphics::Vulkan& vulkan, const Graphics::Frame::Descriptor& frameDesc) 
			: uploader(vulkan, frameDesc)
		{
		}

		~Open() = default;

		Zuazo::Video process(const FFmpeg::Frame& frame) {
			auto result = uploader.acquireFrame();
			assert(result);
				
			const auto& dstBuffers = result->getPixelData();
			const auto srcBuffers = frame.getData();
			const auto srcLinesizes = frame.getLineSizes();
			const auto planeCount = Math::min(dstBuffers.size(), srcBuffers.size());
			const auto resolution = frame.getResolution();
			assert(srcBuffers.size() == srcLinesizes.size());

			//Copy all the data into it
			for(size_t plane = 0; plane < planeCount; plane++) {
				//Copy plane by plane
				const auto byteWidth = av_image_get_linesize(
					static_cast<AVPixelFormat>(frame.getPixelFormat()),
					resolution.width,
					plane
				);

				const auto height = getSubsampledHeight(frame.getPixelFormat(), resolution.height, plane);

				assert(dstBuffers[plane].size() >= byteWidth * height);
				av_image_copy_plane(
					reinterpret_cast<uint8_t*>(dstBuffers[plane].data()),
					byteWidth, //Vulkan does not leave spacing between lines
					reinterpret_cast<const uint8_t*>(srcBuffers[plane]),
					srcLinesizes[plane],
					byteWidth,
					height
				);
			}

			result->flush();
			return result;
		}

		void setFrameDescriptor(const Graphics::Frame::Descriptor& frameDesc) {
			uploader = Graphics::Uploader(
				uploader.getVulkan(), 
				frameDesc
			);
		}

	private:
		static uint32_t getSubsampledHeight(FFmpeg::PixelFormat pixFmt, uint32_t height, int plane) {
			//Based on: http://www.ffmpeg.org/doxygen/trunk/imgutils_8c_source.html#l00111
			const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(pixFmt));
			const uint32_t s = (plane == 1 || plane == 2) ? desc->log2_chroma_h : 0;
			return (height + (1 << s) - 1) >> s;
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
					uploader.setVideoModeCompatibility(
						createVideoModeCompatibility(*newFrame)
					);
				}
			} else if(oldFrame && !newFrame) {
				//Frame has become invalid
				opened.reset();
				videoOut.reset();
				uploader.setVideoModeCompatibility({});
			} else if(!oldFrame && newFrame) {
				//Frame has become valid
				assert(newFrame);
				uploader.setVideoModeCompatibility(
					createVideoModeCompatibility(*newFrame)
				);
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
				opened->setFrameDescriptor(videoMode.getFrameDescriptor());
			} else if (opened && !isValid) {
				//It has become invalid
				opened.reset();
				videoOut.reset();
			} else if(!opened && isValid) {
				//It has become valid
				opened = Utils::makeUnique<Open>(
					uploader.getInstance().getVulkan(),
					videoMode.getFrameDescriptor()
				);
			}
		}
	}
	
private:
	static std::vector<VideoMode> createVideoModeCompatibility(const FFmpeg::Frame& frame) 
	{	
		std::vector<VideoMode> result;

		const auto frameResolution = frame.getResolution();
		const auto framePixelAspectRatio = frame.getPixelAspectRatio();
		const auto frameColorPrimaries = frame.getColorPrimaries();
		const auto frameColorSpace = frame.getColorSpace();
		const auto frameColorTransferFunction = frame.getColorTransferCharacteristic();
		const auto frameColorRange = frame.getColorRange();
		const auto framePixelFormat = frame.getPixelFormat();

		const auto fmtConversion = FFmpeg::fromFFmpeg(framePixelFormat);
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

	static Output::PullCallback createPullCallback(FFmpegUploader& uplo) {
		return [&uplo] (Output&) -> void {
			uplo.update();
		};
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


}
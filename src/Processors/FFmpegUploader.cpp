#include <zuazo/Processors/FFmpegUploader.h>

#include <zuazo/Utils/Functions.h>
#include <zuazo/Math/Functions.h>
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
 * FFmpegUploader::Impl
 */

struct FFmpegUploader::Impl {
	struct Open {
		using FrameCharacteristics = std::tuple<Resolution,
												AspectRatio,
												FFmpeg::ColorPrimaries,
												FFmpeg::ColorSpace,
												FFmpeg::ColorTransferCharacteristic,
												FFmpeg::ColorRange,
												FFmpeg::PixelFormat >;

		FrameCharacteristics	lastFrameCharacteristics;
		Graphics::Uploader		uploader;

		Open(const Graphics::Vulkan& vulkan, const VideoMode& videoMode) 
			: lastFrameCharacteristics(createFrameCharacteristics())
			, uploader(vulkan, videoMode ? videoMode.getFrameDescriptor() : Graphics::Frame::Descriptor())
		{
		}

		~Open() = default;

		Zuazo::Video process(FFmpegUploader& ffmpegUploader, const FFmpeg::Video& frame) {
			assert(frame);

			//Evaluate if frame characteristics have changed
			const auto frameCharacteristics = createFrameCharacteristics(*frame);
			if(lastFrameCharacteristics != frameCharacteristics) {
				//Frame characteristics have changed
				lastFrameCharacteristics = frameCharacteristics;
				ffmpegUploader.setVideoModeCompatibility(getVideoModeCompatibility(frameCharacteristics));
			}

			//Only proceed if there it has a valid videoMode
			if(ffmpegUploader.getVideoMode()) {
				auto result = uploader.acquireFrame();
				assert(result);
				
				const auto& dstBuffers = result->getPixelData();
				const auto srcBuffers = frame->getData();
				const auto srcLinesizes = frame->getLineSizes();
				const auto planeCount = Math::min(dstBuffers.size(), srcBuffers.size());
				const auto resolution = frame->getResolution();
				assert(srcBuffers.size() == srcLinesizes.size());

				//Copy all the data into it
				for(size_t plane = 0; plane < planeCount; plane++) {
					//Copy plane by plane
					const auto byteWidth = av_image_get_linesize(
						static_cast<AVPixelFormat>(frame->getPixelFormat()),
						resolution.width,
						plane
					);

					const auto height = getSubsampledHeight(frame->getPixelFormat(), resolution.height, plane);

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

			//Fail
			return Zuazo::Video();
		}

		void videoModeCallback(const VideoMode& videoMode) {
			if(videoMode) {
				uploader = Graphics::Uploader(
					uploader.getVulkan(), 
					videoMode.getFrameDescriptor()
				);
			}
		}

	private:
		static FrameCharacteristics createFrameCharacteristics() {
			return std::make_tuple(
				Resolution(0, 0),
				AspectRatio(1, 1),
				FFmpeg::ColorPrimaries::NONE,
				FFmpeg::ColorSpace::NONE,
				FFmpeg::ColorTransferCharacteristic::NONE,
				FFmpeg::ColorRange::NONE,
				FFmpeg::PixelFormat::NONE
			);
		}

		static FrameCharacteristics createFrameCharacteristics(const FFmpeg::Frame& frame) {
			return std::make_tuple(
				frame.getResolution(),
				frame.getPixelAspectRatio(),
				frame.getColorPrimaries(),
				frame.getColorSpace(),
				frame.getColorTransferCharacteristic(),
				frame.getColorRange(),
				frame.getPixelFormat()
			);
		}

		static std::vector<VideoMode> getVideoModeCompatibility(const FrameCharacteristics& frameChar) {
			return std::vector<VideoMode> { std::apply(&Open::createVideoMode, frameChar) };
		}

		static VideoMode createVideoMode(	Resolution res,
											AspectRatio par,
											FFmpeg::ColorPrimaries prim,
											FFmpeg::ColorSpace space,
											FFmpeg::ColorTransferCharacteristic trc,
											FFmpeg::ColorRange range,
											FFmpeg::PixelFormat fmt ) 
		{	
			const auto fmtConversion = FFmpeg::fromFFmpeg(fmt);
			const auto defaultColorModel = fmtConversion.isYCbCr ? ColorModel::BT709 : ColorModel::RGB;

			const auto resolution = res;
			const auto pixelAspectRatio = (par != AspectRatio(0, 1)) ? par : AspectRatio(1, 1);
			const auto colorPrimaries = (prim != FFmpeg::ColorPrimaries::NONE) ? FFmpeg::fromFFmpeg(prim) : ColorPrimaries::BT709;
			const auto colorModel = (space != FFmpeg::ColorSpace::NONE) ? FFmpeg::fromFFmpeg(space) : defaultColorModel;
			const auto colorTransferFunction = (trc != FFmpeg::ColorTransferCharacteristic::NONE) ? FFmpeg::fromFFmpeg(trc) : ColorTransferFunction::BT709;
			const auto colorSubsampling = fmtConversion.colorSubsampling;
			const auto colorRange = (range != FFmpeg::ColorRange::NONE) ? FFmpeg::fromFFmpeg(range) : ColorRange::FULL;
			const auto colorFormat = fmtConversion.colorFormat;

			return VideoMode(
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
		}

		static uint32_t getSubsampledHeight(FFmpeg::PixelFormat pixFmt, uint32_t height, int plane) {
			//Based on: http://www.ffmpeg.org/doxygen/trunk/imgutils_8c_source.html#l00111
			const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(pixFmt));
			const uint32_t s = (plane == 1 || plane == 2) ? desc->log2_chroma_h : 0;
			return (height + (1 << s) - 1) >> s;
		}
	};

	using Input = Signal::Input<FFmpeg::Video>;
	using Output = Signal::Output<Zuazo::Video>;

	std::reference_wrapper<FFmpegUploader> owner;

	Input 					videoIn;
	Output					videoOut;

	std::unique_ptr<Open> 	opened;

	Impl(FFmpegUploader& uploader)
		: owner(uploader)
		, videoIn(std::string(Signal::makeInputName<FFmpeg::Video>()))
		, videoOut(std::string(Signal::makeOutputName<Zuazo::Video>()), createPullCallback(uploader))
	{
	}

	~Impl() = default;

	void moved(ZuazoBase& base) {
		owner = static_cast<FFmpegUploader&>(base);
		videoOut.setPullCallback(createPullCallback(owner));
	}

	void open(ZuazoBase& base) {
		assert(!opened);
		const auto& ffmpegUploader = static_cast<FFmpegUploader&>(base);
		assert(&owner.get() == &ffmpegUploader);

		opened = Utils::makeUnique<Open>(
			ffmpegUploader.getInstance().getVulkan(),
			ffmpegUploader.getVideoMode()
		);
	}

	void close(ZuazoBase&) {
		assert(opened);
		opened.reset();
		videoIn.reset();
		videoOut.reset();
	}

	void update() {
		if(opened){
			if(videoIn.hasChanged()) {
				const auto& frame = videoIn.pull();
				videoOut.push(frame ? opened->process(owner, frame) : Zuazo::Video());
			}
		} 
	}

	void videoModeCallabck(VideoBase&, const VideoMode& videoMode) {
		if(opened) opened->videoModeCallback(videoMode);
	}

private:
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
	: ZuazoBase(instance, std::move(name))
	, VideoBase(std::move(videoMode))
	, m_impl({}, *this)
{
	setMoveCallback(std::bind(&Impl::moved, std::ref(*m_impl), std::placeholders::_1));
	setOpenCallback(std::bind(&Impl::open, std::ref(*m_impl), std::placeholders::_1));
	setCloseCallback(std::bind(&Impl::close, std::ref(*m_impl), std::placeholders::_1));
	setUpdateCallback(std::bind(&Impl::update, std::ref(*m_impl)));
	setVideoModeCallback(std::bind(&Impl::videoModeCallabck, std::ref(*m_impl), std::placeholders::_1, std::placeholders::_2));

	registerPads( {m_impl->videoIn, m_impl->videoOut} );
}

FFmpegUploader::FFmpegUploader(FFmpegUploader&& other) = default;

FFmpegUploader::~FFmpegUploader() = default;

FFmpegUploader& FFmpegUploader::operator=(FFmpegUploader&& other) = default;


}
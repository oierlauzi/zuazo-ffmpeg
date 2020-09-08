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

		Open(const Graphics::Vulkan& vulkan) 
			: lastFrameCharacteristics(createFrameCharacteristics())
			, uploader(createUploader(vulkan, lastFrameCharacteristics))
		{
		}

		~Open() = default;

		Zuazo::Video process(const FFmpeg::Video& frame) {
			assert(frame);

			//Evaluate if frame characteristics have changed
			const auto frameCharacteristics = createFrameCharacteristics(*frame);
			if(lastFrameCharacteristics != frameCharacteristics) {
				//Frame characteristics have changed. Recreate the uploader
				const auto frameDesc = convertFrameCharacteristics(frameCharacteristics);
				if(!isValid(frameDesc)) {
					return Zuazo::Video(); //Invalid conversion
				}

				uploader = Graphics::Uploader(uploader.getVulkan(), frameDesc);
				lastFrameCharacteristics = frameCharacteristics;
			}

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

	private:
		static Graphics::Uploader createUploader(const Graphics::Vulkan& vulkan, const FrameCharacteristics& frameChar) {
			return Graphics::Uploader(
				vulkan,
				convertFrameCharacteristics(frameChar)
			);
		}

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

		static FrameCharacteristics createFrameCharacteristics(const FFmpeg::CodecParameters& codecPar) {
			return std::make_tuple(
				codecPar.getResolution(),
				codecPar.getPixelAspectRatio(),
				codecPar.getColorPrimaries(),
				codecPar.getColorSpace(),
				codecPar.getColorTransferCharacteristic(),
				codecPar.getColorRange(),
				codecPar.getPixelFormat()
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

		static Graphics::Frame::Descriptor convertFrameCharacteristics(const FrameCharacteristics& frameChar) {
			return std::apply(&Open::createFrameDescriptor, frameChar);
		}

		static Graphics::Frame::Descriptor createFrameDescriptor(	Resolution res,
																	AspectRatio par,
																	FFmpeg::ColorPrimaries prim,
																	FFmpeg::ColorSpace space,
																	FFmpeg::ColorTransferCharacteristic trc,
																	FFmpeg::ColorRange range,
																	FFmpeg::PixelFormat fmt ) 
		{	
			const auto fmtConversion = FFmpeg::fromFFmpeg(fmt);
			const auto defaultColorModel = fmtConversion.isYCbCr ? ColorModel::BT709 : ColorModel::RGB;

			return Graphics::Frame::Descriptor {
				res,
				par,
				(prim != FFmpeg::ColorPrimaries::NONE) ? FFmpeg::fromFFmpeg(prim) : ColorPrimaries::BT709,
				(space != FFmpeg::ColorSpace::NONE) ? FFmpeg::fromFFmpeg(space) : defaultColorModel,
				(trc != FFmpeg::ColorTransferCharacteristic::NONE) ? FFmpeg::fromFFmpeg(trc) : ColorTransferFunction::BT709,
				fmtConversion.colorSubsampling,
				(range != FFmpeg::ColorRange::NONE) ? FFmpeg::fromFFmpeg(range) : ColorRange::FULL,
				fmtConversion.colorFormat
			};
		}

		static uint32_t getSubsampledHeight(FFmpeg::PixelFormat pixFmt, uint32_t height, int plane) {
			//Based on: http://www.ffmpeg.org/doxygen/trunk/imgutils_8c_source.html#l00111
			const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(pixFmt));
			const uint32_t s = (plane == 1 || plane == 2) ? desc->log2_chroma_h : 0;
			return (height + (1 << s) - 1) >> s;
		}

		static bool isValid(const Graphics::Frame::Descriptor& desc) {
			return 	desc.resolution &&
					desc.pixelAspectRatio &&
					desc.colorPrimaries != ColorPrimaries::NONE &&
					desc.colorModel != ColorModel::NONE &&
					desc.colorTransferFunction != ColorTransferFunction::NONE &&
					desc.colorSubsampling != ColorSubsampling::NONE &&
					desc.colorRange != ColorRange::NONE &&
					desc.colorFormat != ColorFormat::NONE ;
		}
	};

	using Input = Signal::Input<FFmpeg::Video>;
	using Output = Signal::Output<Zuazo::Video>;

	Input 					videoIn;
	Output					videoOut;

	std::unique_ptr<Open> 	opened;

	Impl(FFmpegUploader& uploader)
		: videoIn(std::string(Signal::makeInputName<FFmpeg::Video>()))
		, videoOut(std::string(Signal::makeOutputName<Zuazo::Video>()), createPullCallback(uploader))
	{
	}

	~Impl() = default;

	void moved(ZuazoBase& base) {
		auto& uploader = static_cast<FFmpegUploader&>(base);
		videoOut.setPullCallback(createPullCallback(uploader));
	}

	void open(ZuazoBase& base) {
		assert(!opened);
		opened = Utils::makeUnique<Open>(base.getInstance().getVulkan());
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
				videoOut.push(frame ? opened->process(frame) : Zuazo::Video());
			}
		} 
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

FFmpegUploader::FFmpegUploader(Instance& instance, std::string name)
	: ZuazoBase(instance, std::move(name))
	, m_impl({}, *this)
{
	setMoveCallback(std::bind(&Impl::moved, std::ref(*m_impl), std::placeholders::_1));
	setOpenCallback(std::bind(&Impl::open, std::ref(*m_impl), std::placeholders::_1));
	setCloseCallback(std::bind(&Impl::close, std::ref(*m_impl), std::placeholders::_1));
	setUpdateCallback(std::bind(&Impl::update, std::ref(*m_impl)));

	registerPads( {m_impl->videoIn, m_impl->videoOut} );
}

FFmpegUploader::FFmpegUploader(FFmpegUploader&& other) = default;

FFmpegUploader::~FFmpegUploader() = default;

FFmpegUploader& FFmpegUploader::operator=(FFmpegUploader&& other) = default;


}
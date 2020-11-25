#include <zuazo/Processors/FFmpegDecoder.h>

#include "../FFmpeg/CodecContext.h"

#include <zuazo/Utils/Functions.h>
#include <zuazo/Utils/Pool.h>
#include <zuazo/Signal/Input.h>
#include <zuazo/Signal/Output.h>
#include <zuazo/FFmpeg/Frame.h>
#include <zuazo/FFmpeg/Signals.h>
#include <zuazo/FFmpeg/FFmpegConversions.h>

#include <memory>
#include <queue>
#include <cassert>

extern "C" {
	#include <libavcodec/avcodec.h>
}

namespace Zuazo::Processors {

/*
 * FFmpegDecoderImpl
 */

struct FFmpegDecoderImpl {
	struct Open {
		using PacketQueue = std::queue<FFmpeg::PacketStream>;
		using FramePool = Utils::Pool<FFmpeg::Frame>;

		const AVCodec*			codec;
		FFmpeg::CodecContext	codecContext;
		
		PacketQueue				packetQueue;
		FramePool				framePool;

		inline static const auto flushPacket = FFmpeg::Packet();

		Open(	const FFmpeg::CodecParameters& codecPar,
				void* opaque,
				int threadCount, 
				FFmpeg::ThreadType threadType ) 
			: codec(findDecoder(codecPar))
			, codecContext(codec)
			, packetQueue()
			, framePool()
		{
			if(codecContext.setParameters(codecPar) < 0) {
				return; //ERROR
			}

			codecContext.setOpaque(opaque);
			codecContext.setPixelFormatNegotiationCallback(FFmpegDecoderImpl::pixelFormatNegotiationCallback);

			codecContext.setThreadCount(threadCount);
			codecContext.setThreadType(threadType);

			if(codecContext.open(codec) != 0) {
				return; //ERROR
			}
		}

		~Open() = default;

		FFmpeg::FrameStream decode(const FFmpegDecoder::DemuxCallback& demuxCbk) {
			auto frame = framePool.acquire();
			assert(frame);

			//Unref the previous contents so that in case of failure, garbage isn't pushed.
			//frame->unref(); //done by readFrame()

			int readError;
			while((readError = codecContext.readFrame(*frame)) != 0) {
				switch(readError) {
				case AVERROR(EAGAIN):
					//In order to decode a frame we need another packet. Retrieve it from the queue
					while(packetQueue.empty()) demuxCbk(); //If there are no elements on the queue, populate it.
					assert(!packetQueue.empty());

					assert(packetQueue.front());
					if(codecContext.sendPacket(*packetQueue.front()) == 0) {
						//Succeded sending this packet. Remove it from the queue
						packetQueue.pop();
					}

					break;

				default:
					//Unknown error
					return FFmpeg::FrameStream();
				}
			}

			return frame;
		}

		void read(const FFmpeg::PacketStream& pkt) {
			assert(pkt);
			packetQueue.push(pkt);
		}

		void flush() {
			//Empty the packet queue
			while(packetQueue.size() > 0) packetQueue.pop();
			codecContext.flush();
		}

	private:
		static const AVCodec* findDecoder(const FFmpeg::CodecParameters& codecPar) {
			const auto id = codecPar.getCodecId();
			return avcodec_find_decoder(static_cast<AVCodecID>(id));
		}

	};

	using Input = Signal::Input<FFmpeg::PacketStream>;
	using Output = Signal::Output<FFmpeg::FrameStream>;

	std::reference_wrapper<FFmpegDecoder> owner;

	Input 							packetIn;
	Output							frameOut;

	FFmpeg::CodecParameters			codecParameters;
	int								threadCount;
	FFmpeg::ThreadType				threadType;

	FFmpegDecoder::PixelFormatNegotiationCallback pixFmtCallback;
	FFmpegDecoder::DemuxCallback	demuxCallback;

	std::unique_ptr<Open> 			opened;

	FFmpegDecoderImpl(	FFmpegDecoder& owner, 
						FFmpeg::CodecParameters codecPar, 
						FFmpegDecoder::PixelFormatNegotiationCallback pixFmtCbk,
						FFmpegDecoder::DemuxCallback demuxCbk) 
		: owner(owner)
		, codecParameters(std::move(codecPar))
		, threadCount(1)
		, threadType(FFmpeg::ThreadType::NONE)
		, pixFmtCallback(std::move(pixFmtCbk))
		, demuxCallback(std::move(demuxCbk))
	{
	}

	~FFmpegDecoderImpl() = default;

	void moved(ZuazoBase& base) {
		owner = static_cast<FFmpegDecoder&>(base);
	}


	void open(ZuazoBase& base) {
		const auto& decoder = static_cast<FFmpegDecoder&>(base);
		assert(&decoder == &owner.get()); (void)(decoder);

		opened = Utils::makeUnique<Open>(
			codecParameters, 
			this, 
			threadCount, 
			threadType
		);
	}

	void close(ZuazoBase& base) {
		const auto& decoder = static_cast<FFmpegDecoder&>(base);
		assert(&decoder == &owner.get()); (void)(decoder);

		opened.reset();
		packetIn.reset();
		frameOut.reset();
	}

	void update() {
		if(opened) {
			frameOut.push(opened->decode(demuxCallback));
		}
	}

	void readPacket() {
		if(opened && packetIn.hasChanged()) {
			opened->read(packetIn.pull());
		}
	}

	void flush() {
		if(opened) opened->flush();
		frameOut.reset();
	}


	void setCodecParameters(FFmpeg::CodecParameters codecPar) {
		codecParameters = std::move(codecPar);
		opened->codecContext.setParameters(codecPar);
	}

	const FFmpeg::CodecParameters& getCodecParameters() const {
		return codecParameters;
	}



	void setThreadCount(int cnt) {
		threadCount = cnt;
		if(opened) opened->codecContext.setThreadCount(threadCount);
	}

	int getThreadCount() const {
		return threadCount;
	}	


	void setThreadType(FFmpeg::ThreadType type) {
		threadType = type;
		if(opened) opened->codecContext.setThreadType(threadType);
	}

	FFmpeg::ThreadType getThreadType() const {
		return threadType;
	}


	void setPixelFormatNegotiationCallback(FFmpegDecoder::PixelFormatNegotiationCallback cbk) {
		pixFmtCallback = std::move(cbk);
	}

	const FFmpegDecoder::PixelFormatNegotiationCallback& getPixelFormatNegotiationCallback() const {
		return pixFmtCallback;
	}


	void setDemuxCallback(FFmpegDecoder::DemuxCallback cbk) {
		demuxCallback = std::move(cbk);
	}

	const FFmpegDecoder::DemuxCallback& getDemuxCallback() const {
		return demuxCallback;
	}

private:
	static FFmpeg::PixelFormat pixelFormatNegotiationCallback(	FFmpeg::CodecContext::Handle codecContext, 
																const FFmpeg::PixelFormat* formats ) 
	{
		assert(codecContext);
		assert(formats);

		auto* decoder = static_cast<FFmpegDecoderImpl*>(codecContext->opaque);
		assert(decoder);
		const auto result = decoder->pixFmtCallback ? decoder->pixFmtCallback(decoder->owner, formats) : *formats;

		//If it is a hardware format, a context needs to be created
		if(FFmpeg::isHardwarePixelFormat(result)) {
			const auto type = getHardwareDeviceType(result);

			//Ensure that it is freed, as it might have been allocated previously
			if(codecContext->hw_device_ctx) {
				av_buffer_unref(&(codecContext->hw_device_ctx));
			}

			//Create the new context
			av_hwdevice_ctx_create(
				&(codecContext->hw_device_ctx),
				static_cast<AVHWDeviceType>(type),
				nullptr, nullptr, 0
			);
		}

		return result;
	}

};



/*
 * FFmpegDecoder
 */

FFmpegDecoder::FFmpegDecoder(	Instance& instance, 
								std::string name, 
								FFmpeg::CodecParameters codecPar,
								PixelFormatNegotiationCallback pixFmtCbk,
								DemuxCallback demuxCbk )
	: Utils::Pimpl<FFmpegDecoderImpl>({}, *this, std::move(codecPar), std::move(pixFmtCbk), std::move(demuxCbk))
	, ZuazoBase(
		instance, 
		std::move(name),
		{ (*this)->packetIn, (*this)->frameOut },
		std::bind(&FFmpegDecoderImpl::moved, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegDecoderImpl::open, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegDecoderImpl::close, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegDecoderImpl::update, std::ref(**this)) )
	, Signal::ProcessorLayout<FFmpeg::PacketStream, FFmpeg::FrameStream>(makeProxy((*this)->packetIn), makeProxy((*this)->frameOut))
{
}

FFmpegDecoder::FFmpegDecoder(FFmpegDecoder&& other) = default;

FFmpegDecoder::~FFmpegDecoder() = default;

FFmpegDecoder& FFmpegDecoder::operator=(FFmpegDecoder&& other) = default;



void FFmpegDecoder::readPacket() {
	return (*this)->readPacket();
}

void FFmpegDecoder::flush() {
	(*this)->flush();
}


void FFmpegDecoder::setCodecParameters(FFmpeg::CodecParameters codecPar) {
	(*this)->setCodecParameters(std::move(codecPar));
}

const FFmpeg::CodecParameters& FFmpegDecoder::getCodecParameters() const {
	return (*this)->getCodecParameters();
}


void FFmpegDecoder::setThreadCount(int cnt) {
	(*this)->setThreadCount(cnt);
}

int FFmpegDecoder::getThreadCount() const {
	return (*this)->getThreadCount();
}	


void FFmpegDecoder::setThreadType(FFmpeg::ThreadType type) {
	(*this)->setThreadType(type);
}

FFmpeg::ThreadType FFmpegDecoder::getThreadType() const {
	return (*this)->getThreadType();
}


void FFmpegDecoder::setPixelFormatNegotiationCallback(PixelFormatNegotiationCallback cbk) {
	(*this)->setPixelFormatNegotiationCallback(std::move(cbk));
}

const FFmpegDecoder::PixelFormatNegotiationCallback& FFmpegDecoder::getPixelFormatNegotiationCallback() const {
	return (*this)->getPixelFormatNegotiationCallback();
}


void FFmpegDecoder::setDemuxCallback(DemuxCallback cbk) {
	(*this)->setDemuxCallback(std::move(cbk));
}

const FFmpegDecoder::DemuxCallback& FFmpegDecoder::getDemuxCallback() const {
	return (*this)->getDemuxCallback();
}

}
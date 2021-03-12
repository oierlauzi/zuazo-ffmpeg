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
				bool hwAccelEnabled,
				FFmpeg::ThreadType threadType,
				int threadCount, 
				void* opaque ) 
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

			//Set the hardware device
			if(hwAccelEnabled) {
				static_cast<AVCodecContext*>(codecContext)->hw_device_ctx = createHwDeviceContext(codec);
			}

			//Enable the multithreading
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
						//Succeeded sending this packet. Remove it from the queue
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

		static AVBufferRef* createHwDeviceContext(const AVCodec* codec) {
			AVBufferRef* result = nullptr;

			if(codec) {
				//Iterate through all the hardware configurations
				const AVCodecHWConfig* codecHwConfig;
				for(size_t i = 0; (codecHwConfig = avcodec_get_hw_config(codec, i)) && !result; ++i) {
					assert(codecHwConfig);

					//Check if this initialisation method is supported
					if(codecHwConfig->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) {
						av_hwdevice_ctx_create(
							&result,
							codecHwConfig->device_type,
							nullptr, nullptr, 0
						);
					}
				}
			}

			return result;
		}
	};

	using Input = Signal::Input<FFmpeg::PacketStream>;
	using Output = Signal::Output<FFmpeg::FrameStream>;

	std::reference_wrapper<FFmpegDecoder> owner;

	Input 							packetIn;
	Output							frameOut;

	FFmpeg::CodecParameters			codecParameters;
	bool							hwAccelEnabled;
	FFmpeg::ThreadType				threadType;
	int								threadCount;

	FFmpegDecoder::PixelFormatNegotiationCallback pixFmtCallback;
	FFmpegDecoder::DemuxCallback	demuxCallback;

	std::unique_ptr<Open> 			opened;

	FFmpegDecoderImpl(	FFmpegDecoder& owner, 
						FFmpeg::CodecParameters codecPar, 
						FFmpegDecoder::PixelFormatNegotiationCallback pixFmtCbk,
						FFmpegDecoder::DemuxCallback demuxCbk) 
		: owner(owner)
		, codecParameters(std::move(codecPar))
		, hwAccelEnabled(false)
		, threadType(FFmpeg::ThreadType::NONE)
		, threadCount(1)
		, pixFmtCallback(std::move(pixFmtCbk))
		, demuxCallback(std::move(demuxCbk))
	{
	}

	~FFmpegDecoderImpl() = default;

	void moved(ZuazoBase& base) {
		owner = static_cast<FFmpegDecoder&>(base);
	}


	void open(ZuazoBase& base, std::unique_lock<Instance>* lock = nullptr) {
		const auto& decoder = static_cast<FFmpegDecoder&>(base);
		assert(&decoder == &owner.get()); (void)(decoder);
		assert(!opened);

		//Create in a unlocked environment
		if(lock) lock->unlock(); //FIXME, if it throws, lock must be re-locked
		auto newOpened = Utils::makeUnique<Open>(
			codecParameters, 
			hwAccelEnabled,
			threadType,
			threadCount, 
			this
		);
		if(lock) lock->lock();

		//Apply changes after locking
		opened = std::move(newOpened);

		assert(opened);
	}

	void asyncOpen(ZuazoBase& base, std::unique_lock<Instance>& lock) {
		assert(lock.owns_lock());
		open(base, &lock);
		assert(lock.owns_lock());
	}

	void close(ZuazoBase& base, std::unique_lock<Instance>* lock = nullptr) {
		const auto& decoder = static_cast<FFmpegDecoder&>(base);
		assert(&decoder == &owner.get()); (void)(decoder);
		assert(opened);

		//Apply changes while locked
		auto oldOpened = std::move(opened);
		packetIn.reset();
		frameOut.reset();

		//Destroy in a unlocked environment
		if(lock) lock->unlock();
		oldOpened.reset();
		if(lock) lock->lock();

		assert(!opened);
	}

	void asyncClose(ZuazoBase& base, std::unique_lock<Instance>& lock) {
		assert(lock.owns_lock());
		close(base, &lock);
		assert(lock.owns_lock());
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
	}

	const FFmpeg::CodecParameters& getCodecParameters() const {
		return codecParameters;
	}


	void setHardwareAccelerationEnabled(bool ena) {
		hwAccelEnabled = ena;
	}

	bool getHardwareAccelerationEnabled() const {
		return hwAccelEnabled;
	}

	FFmpeg::HWDeviceType getHardwareDeviceType() const {
		FFmpeg::HWDeviceType result = FFmpeg::HWDeviceType::NONE;

		if(opened) {
			const auto* codecCtx = static_cast<const AVCodecContext*>(opened->codecContext);
			if(codecCtx) {
				if(codecCtx->hw_device_ctx) {
					const auto* hwDeviceCtx = reinterpret_cast<const AVHWDeviceContext*>(codecCtx->hw_device_ctx->data);
					assert(hwDeviceCtx);
					result = static_cast<FFmpeg::HWDeviceType>(hwDeviceCtx->type);
				}
			}
		}

		return result;
	}

	void setThreadType(FFmpeg::ThreadType type) {
		threadType = type;
	}

	FFmpeg::ThreadType getThreadType() const {
		return threadType;
	}


	void setThreadCount(int cnt) {
		threadCount = cnt;
	}

	int getThreadCount() const {
		return threadCount;
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
		return decoder->pixFmtCallback ? decoder->pixFmtCallback(decoder->owner, formats) : *formats;
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
		std::bind(&FFmpegDecoderImpl::open, std::ref(**this), std::placeholders::_1, nullptr),
		std::bind(&FFmpegDecoderImpl::asyncOpen, std::ref(**this), std::placeholders::_1, std::placeholders::_2),
		std::bind(&FFmpegDecoderImpl::close, std::ref(**this), std::placeholders::_1, nullptr),
		std::bind(&FFmpegDecoderImpl::asyncClose, std::ref(**this), std::placeholders::_1, std::placeholders::_2),
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


void FFmpegDecoder::setHardwareAccelerationEnabled(bool ena) {
	(*this)->setHardwareAccelerationEnabled(ena);
}

bool FFmpegDecoder::getHardwareAccelerationEnabled() const {
	return (*this)->getHardwareAccelerationEnabled();
}

FFmpeg::HWDeviceType FFmpegDecoder::getHardwareDeviceType() const {
	return (*this)->getHardwareDeviceType();
}


void FFmpegDecoder::setThreadType(FFmpeg::ThreadType type) {
	(*this)->setThreadType(type);
}

FFmpeg::ThreadType FFmpegDecoder::getThreadType() const {
	return (*this)->getThreadType();
}


void FFmpegDecoder::setThreadCount(int cnt) {
	(*this)->setThreadCount(cnt);
}

int FFmpegDecoder::getThreadCount() const {
	return (*this)->getThreadCount();
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
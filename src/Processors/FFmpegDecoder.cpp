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

			codecContext.setThreadCount(threadCount);
			codecContext.setThreadType(threadType);

			if(codecContext.open(codec) != 0) {
				return; //ERROR
			}
		}

		~Open() = default;

		void update(const FFmpeg::PacketStream& pkt) {
			assert(pkt);
			packetQueue.push(pkt);
		}

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

	Input 							packetIn;
	Output							frameOut;

	FFmpeg::CodecParameters			codecParameters;
	int								threadCount;
	FFmpeg::ThreadType				threadType;

	FFmpegDecoder::DemuxCallback	demuxCallback;

	std::unique_ptr<Open> 			opened;

	FFmpegDecoderImpl(FFmpeg::CodecParameters codecPar, FFmpegDecoder::DemuxCallback demuxCbk) 
		: codecParameters(std::move(codecPar))
		, threadCount(1)
		, threadType(FFmpeg::ThreadType::NONE)
		, demuxCallback(std::move(demuxCbk))
	{
	}

	~FFmpegDecoderImpl() = default;


	void open(ZuazoBase&) {
		opened = Utils::makeUnique<Open>(codecParameters, threadCount, threadType);
	}

	void close(ZuazoBase&) {
		opened.reset();
		packetIn.reset();
		frameOut.reset();
	}

	void update() {
		if(opened && packetIn.hasChanged()) {
			opened->update(packetIn.pull());
		}
	}

	bool decode() {
		if(opened) {
			auto result = opened->decode(demuxCallback);

			if(result) {
				frameOut.push(std::move(result));
				return true;
			}
		}

		return false;
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


	void setDemuxCallback(FFmpegDecoder::DemuxCallback cbk) {
		demuxCallback = std::move(cbk);
	}

	const FFmpegDecoder::DemuxCallback& getDemuxCallback() const {
		return demuxCallback;
	}

};



/*
 * FFmpegDecoder
 */

FFmpegDecoder::FFmpegDecoder(	Instance& instance, 
								std::string name, 
								FFmpeg::CodecParameters codecPar,
								DemuxCallback demuxCbk )
	: Utils::Pimpl<FFmpegDecoderImpl>({}, std::move(codecPar), std::move(demuxCbk))
	, ZuazoBase(
		instance, 
		std::move(name),
		{ (*this)->packetIn, (*this)->frameOut },
		ZuazoBase::MoveCallback(),
		std::bind(&FFmpegDecoderImpl::open, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegDecoderImpl::close, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegDecoderImpl::update, std::ref(**this)) )
	, Signal::ProcessorLayout<FFmpeg::PacketStream, FFmpeg::FrameStream>(makeProxy((*this)->packetIn), makeProxy((*this)->frameOut))
{
}

FFmpegDecoder::FFmpegDecoder(FFmpegDecoder&& other) = default;

FFmpegDecoder::~FFmpegDecoder() = default;

FFmpegDecoder& FFmpegDecoder::operator=(FFmpegDecoder&& other) = default;



bool FFmpegDecoder::decode() {
	return (*this)->decode();
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


void FFmpegDecoder::setDemuxCallback(DemuxCallback cbk) {
	(*this)->setDemuxCallback(std::move(cbk));
}

const FFmpegDecoder::DemuxCallback& FFmpegDecoder::getDemuxCallback() const {
	return (*this)->getDemuxCallback();
}

}
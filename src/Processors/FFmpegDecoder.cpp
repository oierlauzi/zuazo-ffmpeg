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
 * FFmpegDecoder::Impl
 */

struct FFmpegDecoder::Impl {
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

		FFmpeg::Video decode(const DemuxCallback& demuxCbk) {
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
					return FFmpeg::Video();
				}
			}

			return frame;
		}

		void flush() {
			const auto frame = framePool.acquire();
			assert(frame);

			//Empty the packet queue
			while(packetQueue.size() > 0) packetQueue.pop();

			//Flush the codec itself
			//codecContext.sendPacket(flushPacket);
			//while(codecContext.readFrame(*frame) == 0);
			
			codecContext.flush();
		}

	private:
		static const AVCodec* findDecoder(const FFmpeg::CodecParameters& codecPar) {
			const auto id = codecPar.getCodecId();
			return avcodec_find_decoder(static_cast<AVCodecID>(id));
		}

	};

	using Input = Signal::Input<FFmpeg::PacketStream>;
	using Output = Signal::Output<FFmpeg::Video>;

	FFmpeg::CodecParameters	codecParameters;
	int						threadCount;
	FFmpeg::ThreadType		threadType;

	DemuxCallback			demuxCallback;

	Input 					packetIn;
	Output					videoOut;

	std::unique_ptr<Open> 	opened;

	Impl(FFmpeg::CodecParameters codecPar, DemuxCallback demuxCbk) 
		: codecParameters(std::move(codecPar))
		, threadCount(1)
		, threadType(FFmpeg::ThreadType::NONE)
		, demuxCallback(std::move(demuxCbk))
		, packetIn(std::string(Signal::makeInputName<FFmpeg::PacketStream>()))
		, videoOut(std::string(Signal::makeOutputName<FFmpeg::Video>()))
	{
	}

	~Impl() = default;


	void open(ZuazoBase&) {
		opened = Utils::makeUnique<Open>(codecParameters, threadCount, threadType);
	}

	void close(ZuazoBase&) {
		opened.reset();
		packetIn.reset();
		videoOut.reset();
	}

	void update() {
		if(opened && packetIn.hasChanged()) {
			opened->update(packetIn.pull());
		}
	}

	void decode() {
		if(opened) {
			videoOut.push(opened->decode(demuxCallback));
		}
	}

	void flush() {
		if(opened) opened->flush();
		videoOut.reset();
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


	void setDemuxCallback(DemuxCallback cbk) {
		demuxCallback = std::move(cbk);
	}

	const DemuxCallback& getDemuxCallback() const {
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
	: ZuazoBase(instance, std::move(name))
	, m_impl({}, std::move(codecPar), std::move(demuxCbk))
{
	setOpenCallback(std::bind(&Impl::open, std::ref(*m_impl), std::placeholders::_1));
	setCloseCallback(std::bind(&Impl::close, std::ref(*m_impl), std::placeholders::_1));
	setUpdateCallback(std::bind(&Impl::update, std::ref(*m_impl)));

	registerPads( { m_impl->packetIn, m_impl->videoOut } );
}

FFmpegDecoder::FFmpegDecoder(FFmpegDecoder&& other) = default;

FFmpegDecoder::~FFmpegDecoder() = default;

FFmpegDecoder& FFmpegDecoder::operator=(FFmpegDecoder&& other) = default;



void FFmpegDecoder::decode() {
	m_impl->decode();
}

void FFmpegDecoder::flush() {
	m_impl->flush();
}


void FFmpegDecoder::setCodecParameters(FFmpeg::CodecParameters codecPar) {
	m_impl->setCodecParameters(std::move(codecPar));
}

const FFmpeg::CodecParameters& FFmpegDecoder::getCodecParameters() const {
	return m_impl->getCodecParameters();
}


void FFmpegDecoder::setThreadCount(int cnt) {
	m_impl->setThreadCount(cnt);
}

int FFmpegDecoder::getThreadCount() const {
	return m_impl->getThreadCount();
}	


void FFmpegDecoder::setThreadType(FFmpeg::ThreadType type) {
	m_impl->setThreadType(type);
}

FFmpeg::ThreadType FFmpegDecoder::getThreadType() const {
	return m_impl->getThreadType();
}


void FFmpegDecoder::setDemuxCallback(DemuxCallback cbk) {
	m_impl->setDemuxCallback(std::move(cbk));
}

const FFmpegDecoder::DemuxCallback& FFmpegDecoder::getDemuxCallback() const {
	return m_impl->getDemuxCallback();
}

}
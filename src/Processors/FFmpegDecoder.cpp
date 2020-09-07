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

		Open(const FFmpeg::CodecParameters& codecPar) 
			: codec(findDecoder(codecPar))
			, codecContext(codec)
			, packetQueue()
			, framePool()
		{
			if(codecContext.setParameters(codecPar) < 0) {
				return; //ERROR
			}

			if(codecContext.open(codec) != 0) {
				return; //ERROR
			}
		}

		~Open() = default;

		FFmpeg::Video process(const FFmpeg::PacketStream& packet) {
			assert(packet);
			packetQueue.push(packet);

			auto frame = framePool.acquire();
			assert(frame);

			//Unref the previous contents so that in case of failure, garbage isn't pushed.
			//frame->unref(); //done by readFrame()

			//Try to decode a frame
			while(codecContext.readFrame(*frame) == AVERROR(EAGAIN) && !packetQueue.empty()) {
				//In order to decode a frame we need another packet. Retrieve it from the queue
				const auto& packet = packetQueue.front();
				assert(packet); //It must be a valid pointer
				if(codecContext.sendPacket(*packet) == 0) {
					//Succeded sending this packet. Remove it from the queue
					packetQueue.pop();
				}
			}

			//Hopefuly, a new frame has been decoded at this point
			return frame->getResolution() ? frame : FFmpeg::Video();
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

	Input 					packetIn;
	Output					videoOut;

	std::unique_ptr<Open> 	opened;

	Impl(FFmpeg::CodecParameters codecPar) 
		: codecParameters(std::move(codecPar))			
		, packetIn(std::string(Signal::makeInputName<FFmpeg::PacketStream>()))
		, videoOut(std::string(Signal::makeOutputName<FFmpeg::Video>()))
	{
	}

	~Impl() = default;


	void open(ZuazoBase&) {
		opened = Utils::makeUnique<Open>(codecParameters);
	}

	void close(ZuazoBase&) {
		opened.reset();
	}

	void update() {
		if(opened) {
			const auto& packet = packetIn.hasChanged() ? packetIn.pull() : Signal::Output<FFmpeg::PacketStream>::NO_SIGNAL;
			videoOut.push(packet ? opened->process(packet) : FFmpeg::Video());
		}
	}

	void setCodecParameters(FFmpeg::CodecParameters codecPar) {
		codecParameters = std::move(codecPar);
		if(opened) opened->codecContext.setParameters(codecPar);
	}

	const FFmpeg::CodecParameters& getCodecParameters() const {
		return codecParameters;
	}

};



/*
 * FFmpegDecoder
 */

FFmpegDecoder::FFmpegDecoder(Instance& instance, std::string name, FFmpeg::CodecParameters codecPar)
	: ZuazoBase(instance, std::move(name))
	, m_impl({}, std::move(codecPar))
{
	setOpenCallback(std::bind(&Impl::open, std::ref(*m_impl), std::placeholders::_1));
	setCloseCallback(std::bind(&Impl::close, std::ref(*m_impl), std::placeholders::_1));
	setUpdateCallback(std::bind(&Impl::update, std::ref(*m_impl)));

	registerPads( { m_impl->packetIn, m_impl->videoOut } );
}

FFmpegDecoder::FFmpegDecoder(FFmpegDecoder&& other) = default;

FFmpegDecoder::~FFmpegDecoder() = default;

FFmpegDecoder& FFmpegDecoder::operator=(FFmpegDecoder&& other) = default;



void FFmpegDecoder::setCodecParameters(FFmpeg::CodecParameters codecPar) {
	m_impl->setCodecParameters(std::move(codecPar));
}

const FFmpeg::CodecParameters& FFmpegDecoder::getCodecParameters() const {
	return m_impl->getCodecParameters();
}

}
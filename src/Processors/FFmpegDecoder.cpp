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

		using Input = Signal::Input<FFmpeg::PacketStream>;
		using Output = Signal::Output<FFmpeg::Video>;


		const AVCodec*			codec;
		FFmpeg::CodecContext	codecContext;
		
		PacketQueue				packetQueue;
		FramePool				framePool;

		Input 					packetIn;
		Output					videoOut;


		Open(const FFmpeg::CodecParameters& codecPar) 
			: codec(findDecoder(codecPar))
			, codecContext(codec)
			, packetQueue()
			, framePool()
			, packetIn(std::string(Signal::makeInputName<FFmpeg::PacketStream>()))
			, videoOut(std::string(Signal::makeOutputName<FFmpeg::Video>()))
		{
			if(codecContext.setParameters(codecPar) < 0) {
				return; //ERROR
			}

			if(codecContext.open(codec) != 0) {
				return; //ERROR
			}
		}

		~Open() = default;

		void update() {
			if(packetIn.hasChanged()) {
				const auto& packet = packetIn.pull();
				if(packet) packetQueue.push(packet); //Push only if it is valid
			}

			if(packetQueue.empty()) {
				videoOut.reset();
			} else {
				auto frame = framePool.acquire();
				assert(frame);

				//Unref the previous contents so that in case of failure, garbage isn't pushed.
				//frame->unref(); //done by readFrame()

				//Try to decode a frame
				while(codecContext.readFrame(*frame) == AVERROR(EAGAIN)) {
					//In order to decode a frame we need another packet. Retrieve it from the queue
					const auto& packet = packetQueue.front();
					assert(packet); //It must be a valid pointer
					if(codecContext.sendPacket(*packet) == 0) {
						//Succeded sending this packet. Remove it from the queue
						packetQueue.pop();
					}
				}

				//Hopefuly, a new frame has been decoded at this point
				if(frame->getData().data()) {
					videoOut.push(std::move(frame));
				} else {
					//Failed to decode
					videoOut.reset();
				}
			}
		}

	private:
		static const AVCodec* findDecoder(const FFmpeg::CodecParameters& codecPar) {
			const auto id = codecPar.getCodecId();
			return avcodec_find_decoder(static_cast<AVCodecID>(id));
		}

		static const AVCodec* constructOutput(const FFmpeg::CodecParameters& codecPar) {
			const auto id = codecPar.getCodecId();
			return avcodec_find_decoder(static_cast<AVCodecID>(id));
		}

	};

	FFmpeg::CodecParameters	codecParameters;
	std::unique_ptr<Open> 	opened;

	Impl(FFmpeg::CodecParameters codecPar) 
		: codecParameters(std::move(codecPar))
	{
	}

	~Impl() = default;


	void open(ZuazoBase& base) {
		assert(!opened);
		auto& decoder = static_cast<FFmpegDecoder&>(base);

		opened = Utils::makeUnique<Open>(codecParameters);
		decoder.registerPads( { opened->packetIn, opened->videoOut } );
	}

	void close(ZuazoBase& base) {
		assert(opened);
		auto& decoder = static_cast<FFmpegDecoder&>(base);

		decoder.removePad(opened->packetIn);
		decoder.removePad(opened->videoOut);
		opened.reset();
	}

	void update() {
		if(opened) opened->update();
	}
};



/*
 * FFmpegDecoder
 */

FFmpegDecoder::FFmpegDecoder(Instance& instance, std::string name, FFmpeg::CodecParameters codecPar)
	: ZuazoBase(instance, std::move(name))
	, m_impl({}, codecPar)
{
	setOpenCallback(std::bind(&Impl::open, std::ref(*m_impl), std::placeholders::_1));
	setCloseCallback(std::bind(&Impl::close, std::ref(*m_impl), std::placeholders::_1));
	setUpdateCallback(std::bind(&Impl::update, std::ref(*m_impl)));
}

FFmpegDecoder::FFmpegDecoder(FFmpegDecoder&& other) = default;

FFmpegDecoder::~FFmpegDecoder() = default;

FFmpegDecoder& FFmpegDecoder::operator=(FFmpegDecoder&& other) = default;


}
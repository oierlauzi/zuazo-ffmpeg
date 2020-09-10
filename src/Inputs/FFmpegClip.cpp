#include <zuazo/Inputs/FFmpegClip.h>

#include <zuazo/Inputs/FFmpegDemuxer.h>
#include <zuazo/Processors/FFmpegDecoder.h>
#include <zuazo/Processors/FFmpegUploader.h>
#include <zuazo/Chrono.h>
#include <zuazo/Signal/Input.h>
#include <zuazo/Signal/Output.h>
#include <zuazo/Signal/DummyPad.h>
#include <zuazo/FFmpeg/Signals.h>

#include <memory>
#include <utility>

extern "C" {
	#include <libavutil/avutil.h>
	#include <libavutil/mathematics.h>
}

namespace Zuazo::Inputs {

/*
 * FFmpegClip::Impl
 */

struct FFmpegClip::Impl {
	struct Open {
		using DecoderOutput = Signal::Layout::PadProxy<Signal::Output<FFmpeg::Video>>;

		FFmpegDemuxer&				demuxer;
		int							videoStreamIndex;
		int							audioStreamIndex;
		Processors::FFmpegDecoder 	videoDecoder;
		Processors::FFmpegDecoder 	audioDecoder;
		const DecoderOutput&		videoDecoderOutput;
		const DecoderOutput&		audioDecoderOutput;

		Processors::FFmpegUploader 	videoUploader;

		TimePoint					decodedTimeStamp;

		static constexpr auto NO_TS = TimePoint(Duration(-1));

		Open(Inputs::FFmpegDemuxer& demux)
			: demuxer(demux)
			, videoStreamIndex(getStreamIndex(demuxer, Zuazo::FFmpeg::MediaType::VIDEO))
			, audioStreamIndex(getStreamIndex(demuxer, Zuazo::FFmpeg::MediaType::AUDIO))
			, videoDecoder(demuxer.getInstance(), "Video Decoder", getCodecParameters(demuxer, videoStreamIndex), createDemuxCallback(videoStreamIndex))
			, audioDecoder(demuxer.getInstance(), "Audio Decoder", getCodecParameters(demuxer, audioStreamIndex), createDemuxCallback(audioStreamIndex))
			, videoDecoderOutput(getOutput(videoDecoder))
			, audioDecoderOutput(getOutput(audioDecoder))
			, videoUploader(demuxer.getInstance(), "Video Uploader")
			, decodedTimeStamp(NO_TS)
		{
			//Route all the signals
			routePacketStream(demuxer, videoDecoder, videoStreamIndex);
			routePacketStream(demuxer, audioDecoder, audioStreamIndex);
			routeVideoStream(videoDecoder, videoUploader);

			//Enable multithreading
			enableMultithreading(videoDecoder);
			enableMultithreading(audioDecoder);

			//Open them
			open(videoDecoder, videoStreamIndex);
			open(audioDecoder, audioStreamIndex);
			open(videoUploader, videoStreamIndex);
		}

		~Open() = default;

		void decode(TimePoint targetTimeStamp) {
			const auto streams = demuxer.getStreams();
			
			decodedTimeStamp = Math::max(decodedTimeStamp, decode(videoDecoder, videoStreamIndex, videoDecoderOutput, streams, targetTimeStamp));
			decodedTimeStamp = Math::max(decodedTimeStamp, decode(audioDecoder, audioStreamIndex, audioDecoderOutput, streams, targetTimeStamp));
		}

		void flush() {
			flush(videoDecoder, videoStreamIndex);
			flush(audioDecoder, audioStreamIndex);
			decodedTimeStamp = NO_TS;
		}

	private:
		void demuxCallback(int index) {
			assert(isValidIndex(index));

			do {
				demuxer.update();
				const auto lastIndex = demuxer.getLastStreamIndex();

				if(isValidIndex(lastIndex)) {
					if(lastIndex == videoStreamIndex) {
						update(videoDecoder, videoStreamIndex);
					} else if (lastIndex == audioStreamIndex) {
						update(audioDecoder, audioStreamIndex); //TODO currently no need to decode audio
					}
				}
			} while(demuxer.getLastStreamIndex() != index);
		}


		Processors::FFmpegDecoder::DemuxCallback createDemuxCallback(int index) {
			return isValidIndex(index) ? std::bind(&Open::demuxCallback, std::ref(*this), index) : Processors::FFmpegDecoder::DemuxCallback();
		}


		static int getStreamIndex(const Inputs::FFmpegDemuxer& demuxer, Zuazo::FFmpeg::MediaType type) {
			assert(demuxer.isOpen());
			return demuxer.findBestStream(type);
		}

		static Zuazo::FFmpeg::CodecParameters getCodecParameters(const Inputs::FFmpegDemuxer& demuxer, int index) {
			const auto streams = demuxer.getStreams();
			return 	isValidIndex(index)
					? Zuazo::FFmpeg::CodecParameters(streams[index].getCodecParameters())
					: Zuazo::FFmpeg::CodecParameters();
		}

		static const DecoderOutput& getOutput(const Processors::FFmpegDecoder& decoder) {
			return Signal::getOutput<FFmpeg::Video>(decoder);
		}

		static void routePacketStream(Inputs::FFmpegDemuxer& demuxer, Processors::FFmpegDecoder& decoder, int index) {
			if(isValidIndex(index)){
				const auto outputName = Signal::makeOutputName<Zuazo::FFmpeg::PacketStream>(index);
				auto& input = Signal::getInput<Zuazo::FFmpeg::PacketStream>(decoder);
				auto& output = Signal::getOutput<Zuazo::FFmpeg::PacketStream>(demuxer, outputName);
				input << output;
			} 
		}

		static void routeVideoStream(Processors::FFmpegDecoder& decoder, Processors::FFmpegUploader& uploader) {
			auto& input = Signal::getInput<Zuazo::FFmpeg::Video>(uploader);
			auto& output = Signal::getOutput<Zuazo::FFmpeg::Video>(decoder);
			input << output;
		}

		static void open(Processors::FFmpegDecoder& decoder, int index) {
			if(isValidIndex(index)) {
				decoder.open();
			}
		}

		static void open(Processors::FFmpegUploader& uploader, int index) {
			if(isValidIndex(index)) {
				uploader.open();
			}
		}

		static void update(Processors::FFmpegDecoder& decoder, int index) {
			if(isValidIndex(index)) {
				assert(decoder.isOpen());
				decoder.update();
			}
		}

		static TimePoint decode(Processors::FFmpegDecoder& decoder, int index, const DecoderOutput& output, const FFmpegDemuxer::Streams& streams, TimePoint targetTimeStamp) {
			TimePoint decodedTimeStamp = NO_TS;

			if(isValidIndex(index)) {
				assert(decoder.isOpen());
				assert(Math::isInRange(index, 0, static_cast<int>(streams.size() - 1)));		
				const auto& stream = streams[index];

				if(!output.getLastElement()) {
					//Nothing at the output. Decode just in case it is the first time
					decoder.decode();
				}

				//Decode until the target timestamp is reached
				while(output.getLastElement() && (decodedTimeStamp < targetTimeStamp)) {
					assert(output.getLastElement());
					const auto& frame = *output.getLastElement();

					decodedTimeStamp = calculateTimeStamp(stream, frame);
					decoder.decode();
				}
			}

			return decodedTimeStamp;
		}

		static void flush(Processors::FFmpegDecoder& decoder, int index) {
			if(isValidIndex(index)) {
				assert(decoder.isOpen());
				decoder.flush();
			}
		}

		static void enableMultithreading(Processors::FFmpegDecoder& decoder) {
			decoder.setThreadCount(0); //Use all available threads
			decoder.setThreadType(FFmpeg::ThreadType::FRAME); //Don't care the delay
		}


		static bool isValidIndex(int index) {
			return index >= 0;
		}

		static TimePoint calculateTimeStamp(const FFmpeg::StreamParameters& stream, const FFmpeg::Frame& frame) {
			const auto pts = frame.getPTS();
			const auto dur = frame.getPacketDuration();
			const auto timeStamp = pts + (dur > 0 ? dur - 1 : 0);

			const auto timeBase = stream.getTimeBase();
			const auto rescaledTimeStamp = av_rescale_q(
				timeStamp, 
				AVRational{ timeBase.getNumerator(), timeBase.getDenominator() },	//Src time base
				AVRational{ Duration::period::num, Duration::period::den }			//Dst time-base
			);

			return TimePoint(Duration(rescaledTimeStamp));
		}

	};


	std::reference_wrapper<FFmpegClip> 	owner;

	Inputs::FFmpegDemuxer 				demuxer;
	
	Signal::DummyPad<Video>				videoOut;

	std::unique_ptr<Open>				opened;

	Impl(FFmpegClip& ffmpeg, std::string url)
		: owner(ffmpeg)
		, demuxer(ffmpeg.getInstance(), "Demuxer", std::move(url))
		, videoOut(std::string(Signal::makeOutputName<Zuazo::Video>()))
	{
	}

	~Impl() = default;

	void moved(ZuazoBase& base) {
		owner = static_cast<FFmpegClip&>(base);
	}

	void open(ZuazoBase& base) {
		assert(!opened);
		auto& clip = static_cast<FFmpegClip&>(base);
		assert(&owner.get() == &clip);

		demuxer.open();
		opened = Utils::makeUnique<Open>(demuxer);

		//Route the output signal
		videoOut << Signal::getOutput<Zuazo::Video>(opened->videoUploader);

		clip.setDuration(demuxer.getDuration() != Duration() ? demuxer.getDuration() : Duration::max());
		clip.setTimeStep((opened->videoStreamIndex >=0) ? getPeriod(demuxer.getStreams()[opened->videoStreamIndex].getRealFrameRate()) : Duration());
		clip.setTime(clip.getTime()); //Ensure time point is within limits
		clip.enableRegularUpdate(Instance::INPUT_PRIORITY);

		refresh(clip); //Ensure that the first frame has been decoded
	}

	void close(ZuazoBase& base) {
		assert(opened);
		auto& clip = static_cast<FFmpegClip&>(base);
		assert(&owner.get() == &clip);

		clip.disableRegularUpdate();
		clip.setDuration(Duration::max());
		clip.setTimeStep(Duration());


		opened.reset();
		demuxer.close();
	}

	void update() {
		FFmpegClip& clip = owner;
		const auto delta = clip.getInstance().getDeltaT();
		clip.advance(delta);
	}

	void refresh(ClipBase& base) {
		if(opened) {
			auto& clip = static_cast<FFmpegClip&>(base);
			assert(&owner.get() == &clip);

			const auto targetTimeStamp = clip.getTime();
			const auto delta = targetTimeStamp - opened->decodedTimeStamp;

			if(delta < Duration(0)) {
				//Time has gone back!
				demuxer.seek(targetTimeStamp, FFmpeg::SeekFlags::BACKWARD);
				demuxer.flush();
				opened->flush();
			}

			opened->decode(targetTimeStamp);
			if(opened->decodedTimeStamp < targetTimeStamp) {
				//Could not decode til the end
				clip.setDuration(opened->decodedTimeStamp.time_since_epoch());
			}

			printf("%lf\n", static_cast<double>(opened->decodedTimeStamp.time_since_epoch().count()) / clip.getDuration().count());
		}
	}

private:

};


/*
 * FFmpegClip
 */

FFmpegClip::FFmpegClip(	Instance& instance, 
						std::string name, 
						VideoMode videoMode,
						std::string url )
	: ZuazoBase(instance, std::move(name))
	, VideoBase(std::move(videoMode))
	, ClipBase(Duration::max(), Duration())
	, m_impl({}, *this, std::move(url))
{
	setMoveCallback(std::bind(&Impl::moved, std::ref(*m_impl), std::placeholders::_1));
	setOpenCallback(std::bind(&Impl::open, std::ref(*m_impl), std::placeholders::_1));
	setCloseCallback(std::bind(&Impl::close, std::ref(*m_impl), std::placeholders::_1));
	setUpdateCallback(std::bind(&Impl::update, std::ref(*m_impl)));
	setRefreshCallback(std::bind(&Impl::refresh, std::ref(*m_impl), std::placeholders::_1));

	registerPad(m_impl->videoOut.getOutput());
}


FFmpegClip::FFmpegClip(FFmpegClip&& other) = default;

FFmpegClip::~FFmpegClip() = default;

FFmpegClip& FFmpegClip::operator=(FFmpegClip&& other) = default;

}
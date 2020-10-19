#include <zuazo/Sources/FFmpegClip.h>

#include <zuazo/Sources/FFmpegDemuxer.h>
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

namespace Zuazo::Sources {

/*
 * FFmpegClipImpl
 */

struct FFmpegClipImpl {
	struct Open {
		using DecoderOutput = Signal::Layout::PadProxy<Signal::Output<FFmpeg::FrameStream>>;

		FFmpegDemuxer&				demuxer;
		int							videoStreamIndex;
		int							audioStreamIndex;
		Processors::FFmpegDecoder 	videoDecoder;
		Processors::FFmpegDecoder 	audioDecoder;

		TimePoint					decodedTimeStamp;

		static constexpr auto NO_TS = TimePoint(Duration(-1));

		Open(Sources::FFmpegDemuxer& demux)
			: demuxer(demux)
			, videoStreamIndex(getStreamIndex(demuxer, Zuazo::FFmpeg::MediaType::VIDEO))
			, audioStreamIndex(getStreamIndex(demuxer, Zuazo::FFmpeg::MediaType::AUDIO))
			, videoDecoder(demuxer.getInstance(), "Video Decoder", getCodecParameters(demuxer, videoStreamIndex), createDemuxCallback(videoStreamIndex))
			, audioDecoder(demuxer.getInstance(), "Audio Decoder", getCodecParameters(demuxer, audioStreamIndex), createDemuxCallback(audioStreamIndex))
			, decodedTimeStamp(NO_TS)
		{
			//Route all the signals
			routePacketStream(demuxer, videoDecoder, videoStreamIndex);
			routePacketStream(demuxer, audioDecoder, audioStreamIndex);

			//Enable multithreading
			enableMultithreading(videoDecoder);
			enableMultithreading(audioDecoder);

			//Open them
			open(videoDecoder, videoStreamIndex);
			open(audioDecoder, audioStreamIndex);
		}

		~Open() = default;

		void decode(TimePoint targetTimeStamp) {
			const auto streams = demuxer.getStreams();
			
			decodedTimeStamp = Math::max(decodedTimeStamp, decode(videoDecoder, videoStreamIndex, streams, targetTimeStamp));
			decodedTimeStamp = Math::max(decodedTimeStamp, decode(audioDecoder, audioStreamIndex, streams, targetTimeStamp));
		}

		void flush() {
			demuxer.flush();
			flush(videoDecoder, videoStreamIndex);
			flush(audioDecoder, audioStreamIndex);
			decodedTimeStamp = NO_TS;
		}

		Rate getFrameRate() {
			const auto streams = demuxer.getStreams();
			return isValidIndex(videoStreamIndex) ? Rate(streams[videoStreamIndex].getRealFrameRate()) : Rate();
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
						update(audioDecoder, audioStreamIndex);
					}
				}
			} while(demuxer.getLastStreamIndex() != index);
		}


		Processors::FFmpegDecoder::DemuxCallback createDemuxCallback(int index) {
			return isValidIndex(index) ? std::bind(&Open::demuxCallback, std::ref(*this), index) : Processors::FFmpegDecoder::DemuxCallback();
		}


		static int getStreamIndex(const Sources::FFmpegDemuxer& demuxer, Zuazo::FFmpeg::MediaType type) {
			assert(demuxer.isOpen());
			return demuxer.findBestStream(type);
		}

		static Zuazo::FFmpeg::CodecParameters getCodecParameters(const Sources::FFmpegDemuxer& demuxer, int index) {
			const auto streams = demuxer.getStreams();
			return 	isValidIndex(index)
					? Zuazo::FFmpeg::CodecParameters(streams[index].getCodecParameters())
					: Zuazo::FFmpeg::CodecParameters();
		}

		static void routePacketStream(Sources::FFmpegDemuxer& demuxer, Processors::FFmpegDecoder& decoder, int index) {
			if(isValidIndex(index)){
				const auto outputName = Signal::makeOutputName<Zuazo::FFmpeg::PacketStream>(index);
				auto& output = Signal::getOutput<Zuazo::FFmpeg::PacketStream>(demuxer, outputName);
				decoder << output;
			} 
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

		static TimePoint decode(Processors::FFmpegDecoder& decoder, int index, const FFmpegDemuxer::Streams& streams, TimePoint targetTimeStamp) {
			TimePoint decodedTimeStamp = NO_TS;

			if(isValidIndex(index)) {
				assert(decoder.isOpen());
				assert(Math::isInRange(index, 0, static_cast<int>(streams.size() - 1)));		
				const auto& stream = streams[index];
				const auto& output = decoder.getOutput();


				//Set the decoded timestamp if available
				if(output.getLastElement()) {
					decodedTimeStamp = calculateTimeStamp(stream, *(output.getLastElement()));
				}

				//Decode until the target timestamp is reached
				while(decodedTimeStamp < targetTimeStamp) {
					if(decoder.decode()) {
						//Succesfuly decoded something!
						assert(output.getLastElement());
						decodedTimeStamp = calculateTimeStamp(stream, *(output.getLastElement()));
					} else {
						//Failed to decode. Exit
						break;
					}
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

	Sources::FFmpegDemuxer 				demuxer;
	Processors::FFmpegUploader 			videoUploader;
	
	Signal::DummyPad<Video>				videoOut;

	std::unique_ptr<Open>				opened;

	FFmpegClipImpl(FFmpegClip& ffmpeg, Instance& instance, std::string url)
		: owner(ffmpeg)
		, demuxer(instance, "Demuxer", std::move(url))
		, videoUploader(instance, "Video Uploader")
		, videoOut(std::string(Signal::makeOutputName<Zuazo::Video>()))
	{
		//Route the output signal
		videoOut << videoUploader;
	}

	~FFmpegClipImpl() = default;

	void moved(ZuazoBase& base) {
		owner = static_cast<FFmpegClip&>(base);
	}

	void open(ZuazoBase& base) {
		assert(!opened);
		auto& clip = static_cast<FFmpegClip&>(base);
		assert(&owner.get() == &clip);

		demuxer.open();
		videoUploader.open();
		opened = Utils::makeUnique<Open>(demuxer);

		//Route the decoder signal
		videoUploader << opened->videoDecoder;

		clip.setDuration(
			demuxer.getDuration() != FFmpeg::Duration() 
			? std::chrono::duration_cast<Duration>(demuxer.getDuration()) 
			: Duration::max()
		);
		clip.setTimeStep(getPeriod(opened->getFrameRate()));
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
		videoUploader.close();
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
			const auto framePeriod = getPeriod(opened->getFrameRate());
			constexpr Duration::rep MAX_UNSKIPPED_FRAMES = 16; //TODO find a way to obtain it from the codec GOP

			if(delta < -framePeriod || delta > framePeriod * MAX_UNSKIPPED_FRAMES) {
				//Time has gone back!
				demuxer.seek(
					std::chrono::duration_cast<FFmpeg::Duration>(targetTimeStamp.time_since_epoch()), 
					FFmpeg::SeekFlags::BACKWARD
				);
				opened->flush();
			}

			opened->decode(targetTimeStamp);
			if(opened->decodedTimeStamp < targetTimeStamp) {
				//Could not decode til the end
				clip.setDuration(opened->decodedTimeStamp.time_since_epoch());
			}
		}
	}

	void videoModeCallback(VideoBase& base, const VideoMode& videoMode) {
		auto& clip = static_cast<FFmpegClip&>(base);
		assert(&owner.get() == &clip);

		videoUploader.setVideoModeLimits(videoMode);
	}

	void videoModeCompatibilityCallback(VideoBase&, std::vector<VideoMode> compatibility) {
		assert(opened);
		assert(opened->videoStreamIndex >= 0);

		//Obtain the framerate from the framerate from the video stream
		const Rate frameRate = opened->getFrameRate();

		//Set the proper framerate in all the VideoModes
		for(auto& vm : compatibility) {
			vm.setFrameRate(Utils::MustBe<Rate>(frameRate));
		}

		//Update the compatibility in the VideoBase
		owner.get().setVideoModeCompatibility(std::move(compatibility));
	}

};


/*
 * FFmpegClip
 */

FFmpegClip::FFmpegClip(	Instance& instance, 
						std::string name, 
						VideoMode videoMode,
						std::string url )
	: Utils::Pimpl<FFmpegClipImpl>({}, *this, instance, std::move(url))
	, ZuazoBase(
		instance, 
		std::move(name),
		{},
		std::bind(&FFmpegClipImpl::moved, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegClipImpl::open, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegClipImpl::close, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegClipImpl::update, std::ref(**this)) )
	, VideoBase(
		std::move(videoMode),
		std::bind(&FFmpegClipImpl::videoModeCallback, std::ref(**this), std::placeholders::_1, std::placeholders::_2) )
	, ClipBase(
		Duration::max(), 
		Duration(),
		std::bind(&FFmpegClipImpl::refresh, std::ref(**this), std::placeholders::_1) )
	, Signal::SourceLayout<Video>((*this)->videoOut.getOutput())
{
	//Add the output
	ZuazoBase::registerPad((*this)->videoOut.getOutput());

	//Setup the compatibility callback
	(*this)->videoUploader.setVideoModeCompatibilityCallback(
		std::bind(&FFmpegClipImpl::videoModeCompatibilityCallback, std::ref(**this), std::placeholders::_1, std::placeholders::_2)
	);
}


FFmpegClip::FFmpegClip(FFmpegClip&& other) = default;

FFmpegClip::~FFmpegClip() = default;

FFmpegClip& FFmpegClip::operator=(FFmpegClip&& other) = default;

}
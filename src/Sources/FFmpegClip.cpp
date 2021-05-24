#include <zuazo/Sources/FFmpegClip.h>

#include <zuazo/FFmpeg/FFmpegConversions.h>
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
#include <thread>
#include <mutex>
#include <condition_variable>

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
		using DecoderOutput = Signal::PadProxy<Signal::Output<FFmpeg::FrameStream>>;

		FFmpegDemuxer&				demuxer;
		int							videoStreamIndex;
		int							audioStreamIndex;
		Processors::FFmpegDecoder 	videoDecoder;
		Processors::FFmpegDecoder 	audioDecoder;

		TimePoint					targetTimeStamp;
		TimePoint					decodedTimeStamp;

		std::thread					decodingThread;
		std::mutex					decodingMutex;
		std::condition_variable		decodingStartCond;
		std::condition_variable		decodingFinishCond;
		bool						decodingComplete;
		bool						decodingThreadExit;

		static constexpr auto NO_TS = TimePoint(Duration(-1));

		Open(Sources::FFmpegDemuxer& demux)
			: demuxer(demux)
			, videoStreamIndex(getStreamIndex(demuxer, Zuazo::FFmpeg::MediaType::VIDEO))
			, audioStreamIndex(getStreamIndex(demuxer, Zuazo::FFmpeg::MediaType::AUDIO))
			, videoDecoder(demuxer.getInstance(), "Video Decoder", getCodecParameters(demuxer, videoStreamIndex), Open::pixelFormatNegotiationCallback,	createDemuxCallback(videoStreamIndex))
			, audioDecoder(demuxer.getInstance(), "Audio Decoder", getCodecParameters(demuxer, audioStreamIndex), {}, 									createDemuxCallback(audioStreamIndex))
			, decodedTimeStamp(NO_TS)
			, decodingComplete(false)
			, decodingThreadExit(false)
		{
			//Route all the signals
			routePacketStream(demuxer, videoDecoder, videoStreamIndex);
			routePacketStream(demuxer, audioDecoder, audioStreamIndex);

			//Enable multithreading and HW acceleration
			configure(videoDecoder);
			configure(audioDecoder);

			//Open them
			open(videoDecoder, videoStreamIndex);
			open(audioDecoder, audioStreamIndex);

			//Start the thread
			decodingThread = std::thread(&Open::decodingThreadFunc, std::ref(*this));
		}

		~Open() {
			//Wait until the thread dies
			std::unique_lock<std::mutex> lock(decodingMutex);
			decodingThreadExit = true;
			decodingStartCond.notify_all();
			lock.unlock();
			decodingThread.join();
		}

		void decode(TimePoint target) {
			std::lock_guard<std::mutex> lock(decodingMutex);

			//Start decoding
			decodingComplete = false;
			targetTimeStamp = target;
			decodingStartCond.notify_all();
		}

		bool waitDecode() {
			std::unique_lock<std::mutex> lock(decodingMutex);

			while(!decodingComplete) {
				decodingFinishCond.wait(lock);
			}

			return decodedTimeStamp >= targetTimeStamp;
		}

		Rate getFrameRate() {
			const auto streams = demuxer.getStreams();
			return isValidIndex(videoStreamIndex) ? Rate(streams[videoStreamIndex].getRealFrameRate()) : Rate();
		}

	private:
		void decodingThreadFunc() {
			std::unique_lock<std::mutex> lock(decodingMutex);

			while(!decodingThreadExit) {
				//Evaluate if flushing is needed
				const auto framePeriod = getPeriod(getFrameRate());
				const auto delta = targetTimeStamp - decodedTimeStamp;
				const auto frameDelta = delta / framePeriod;
			
				constexpr Duration::rep MAX_UNSKIPPED_FRAMES = 16; //TODO find a way to obtain it from the codec GOP
				if(frameDelta < 0 || frameDelta > MAX_UNSKIPPED_FRAMES) {
					//Time delta is too high, seek the demuxer and flush all buffers
					demuxer.seek(
						std::chrono::duration_cast<FFmpeg::Duration>(targetTimeStamp.time_since_epoch()), 
						FFmpeg::SeekFlags::BACKWARD
					);

					demuxer.flush();
					flush(videoDecoder, videoStreamIndex);
					flush(audioDecoder, audioStreamIndex);
				}

				//Decode
				const auto streams = demuxer.getStreams();
				decodedTimeStamp = TimePoint::max();
				if(isValidIndex(videoStreamIndex)) {
					decodedTimeStamp = Math::min(decodedTimeStamp, decode(videoDecoder, videoStreamIndex, streams, targetTimeStamp));
				}
				if(isValidIndex(audioStreamIndex)) {
					decodedTimeStamp = Math::min(decodedTimeStamp, decode(audioDecoder, audioStreamIndex, streams, targetTimeStamp));
				}

				//Wait until decoding is signaled
				decodingComplete = true;
				decodingFinishCond.notify_all();
				decodingStartCond.wait(lock);
			}
		}

		void demuxCallback(int index) {
			assert(isValidIndex(index));

			do {
				demuxer.update();
				const auto lastIndex = demuxer.getLastStreamIndex();

				if(isValidIndex(lastIndex)) {
					if(lastIndex == videoStreamIndex) {
						readPacket(videoDecoder, videoStreamIndex);
					} else if (lastIndex == audioStreamIndex) {
						readPacket(audioDecoder, audioStreamIndex);
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

		static void readPacket(Processors::FFmpegDecoder& decoder, int index) {
			if(isValidIndex(index)) {
				assert(decoder.isOpen());
				decoder.readPacket();
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
					decoder.update();
					if(output.getLastElement()) {
						//Successfully decoded something!
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

		static void configure(Processors::FFmpegDecoder& decoder) {
			decoder.setHardwareAccelerationEnabled(true); //Use hardware accel if possible
			decoder.setThreadCount(0); //Use all available threads
			decoder.setThreadType(FFmpeg::ThreadType::FRAME); //Don't care about the delay
		}

		static bool isValidIndex(int index) {
			return index >= 0;
		}

		static TimePoint calculateTimeStamp(const FFmpeg::StreamParameters& stream, const FFmpeg::Frame& frame) {
			const auto pts = frame.getPTS();
			const auto dur = frame.getPacketDuration();
			const auto timeStamp = pts + dur;

			const auto timeBase = stream.getTimeBase();
			const auto rescaledTimeStamp = (dur != 0 ? -1 : 0) + av_rescale_q(
				timeStamp, 
				AVRational{ timeBase.getNumerator(), timeBase.getDenominator() },	//Src time base
				AVRational{ Duration::period::num, Duration::period::den }			//Dst time-base
			);

			return TimePoint(Duration(rescaledTimeStamp));
		}

		static FFmpeg::PixelFormat pixelFormatNegotiationCallback(	Processors::FFmpegDecoder& decoder,
																	const FFmpeg::PixelFormat* formats ) 
		{
			//Check if a desired hardware accelerated format is present
			for(const auto* f = formats; static_cast<int>(*f) >= 0; ++f) {
				if(	Processors::FFmpegUploader::isSupportedInput(*f) && 
					decoder.getHardwareDeviceType() == getHardwareDeviceType(*f) ) 
				{
					return *f; //A hardware accelerated format was found!
				}
			}

			//No luck with hardware accelerated formats try with normal ones
			for(const auto* f = formats; static_cast<int>(*f) >= 0; ++f) {
				if(Processors::FFmpegUploader::isSupportedInput(*f) && !isHardwarePixelFormat(*f)) {
					return *f; //A compatible format was found!
				}
			}

			return *formats; //Nothing was found :-<
		}

	};


	std::reference_wrapper<FFmpegClip> 	owner;

	Signal::DummyPad<Video>				videoOut;

	Sources::FFmpegDemuxer 				demuxer;
	Processors::FFmpegUploader 			videoUploader;

	std::unique_ptr<Open>				opened;

	FFmpegClipImpl(FFmpegClip& ffmpeg, Instance& instance, std::string url)
		: owner(ffmpeg)
		, videoOut(ffmpeg, std::string(Signal::makeOutputName<Zuazo::Video>()))
		, demuxer(instance, "Demuxer", std::move(url))
		, videoUploader(instance, "Video Uploader")
	{
		//Route the output signal
		videoOut << videoUploader;
		videoUploader.setPreUpdateCallback(std::bind(&FFmpegClipImpl::uploaderPreUpdateCallback, std::ref(*this)));
	}

	~FFmpegClipImpl() = default;

	void moved(ZuazoBase& base) {
		owner = static_cast<FFmpegClip&>(base);
		videoOut.setLayout(base);
		auto& clip = static_cast<FFmpegClip&>(base);
		clip.setRefreshCallback(std::bind(&FFmpegClip::update, std::ref(clip)));
	}

	void open(ZuazoBase& base, std::unique_lock<Instance>* lock = nullptr) {
		auto& clip = static_cast<FFmpegClip&>(base);
		assert(&owner.get() == &clip);
		assert(!opened);

		//Open childs asynchronously if possible
		//May throw! (nothing has been done yet, so don't worry about cleaning)
		if(lock) {
			demuxer.asyncOpen(*lock);
			videoUploader.asyncOpen(*lock);
		} else {
			demuxer.open(); 
			videoUploader.open();
		}

		//Open the decoders
		opened = Utils::makeUnique<Open>(demuxer); //TODO create asynchronously

		//Route the decoder signal
		videoUploader << opened->videoDecoder;

		clip.setDuration(calculateDuration(demuxer));
		clip.setTimeStep(getPeriod(opened->getFrameRate()));
		assert(opened);
	}

	void asyncOpen(ZuazoBase& base, std::unique_lock<Instance>& lock) {
		assert(lock.owns_lock());
		open(base, &lock);
		assert(lock.owns_lock());
	}

	void close(ZuazoBase& base, std::unique_lock<Instance>* lock = nullptr) {
		auto& clip = static_cast<FFmpegClip&>(base);
		assert(&owner.get() == &clip);
		assert(opened);

		clip.setDuration(Duration::max());
		clip.setTimeStep(Duration());

		opened.reset(); //TODO reset asynchronously

		//Close childs asynchronously if possible
		if(lock) {
			videoUploader.asyncClose(*lock);
			demuxer.asyncClose(*lock);
		} else {
			videoUploader.close();
			demuxer.close();
		}

		assert(!opened);
	}

	void asyncClose(ZuazoBase& base, std::unique_lock<Instance>& lock) {
		assert(lock.owns_lock());
		close(base, &lock);
		assert(lock.owns_lock());
	}

	void update() {
		if(opened) {
			auto& clip = owner.get();
			opened->decode(clip.getTime());
		}
	}

	void videoModeCallback(VideoBase& base, const VideoMode& videoMode) {
		auto& clip = static_cast<FFmpegClip&>(base);
		assert(&owner.get() == &clip); (void)(clip);

		videoUploader.setVideoMode(videoMode);
	}

	VideoMode videoModeNegotiationCallback(VideoBase&, std::vector<VideoMode> compatibility) {
		auto& clip = owner.get();
		assert(opened);
		assert(opened->videoStreamIndex >= 0);

		//Obtain the framerate from the framerate from the video stream
		const Rate frameRate = opened->getFrameRate();

		//Set the proper framerate in all the VideoModes
		for(auto& vm : compatibility) {
			vm.setFrameRate(Utils::MustBe<Rate>(frameRate));
		}

		//Update the compatibility in the VideoBase
		clip.setVideoModeCompatibility(std::move(compatibility));
		return clip.getVideoMode();
	}

private:
	void uploaderPreUpdateCallback() {
		//Ensure the decoding has finished before pulling a frame
		assert(opened);

		auto& clip = owner.get();
		if(!opened->waitDecode()) {
			//Could not decode til the end
			clip.setDuration(opened->decodedTimeStamp.time_since_epoch());
		}
	}

	static Duration calculateDuration(FFmpegDemuxer& demux) {
		return demux.getDuration() != FFmpeg::Duration() ? std::chrono::duration_cast<Duration>(demux.getDuration()) : Duration::max();
	}

};


/*
 * FFmpegClip
 */

FFmpegClip::FFmpegClip(	Instance& instance, 
						std::string name, 
						std::string url )
	: Utils::Pimpl<FFmpegClipImpl>({}, *this, instance, std::move(url))
	, ZuazoBase(
		instance, 
		std::move(name),
		{},
		std::bind(&FFmpegClipImpl::moved, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegClipImpl::open, std::ref(**this), std::placeholders::_1, nullptr),
		std::bind(&FFmpegClipImpl::asyncOpen, std::ref(**this), std::placeholders::_1, std::placeholders::_2),
		std::bind(&FFmpegClipImpl::close, std::ref(**this), std::placeholders::_1, nullptr),
		std::bind(&FFmpegClipImpl::asyncClose, std::ref(**this), std::placeholders::_1, std::placeholders::_2),
		std::bind(&FFmpegClipImpl::update, std::ref(**this)) )
	, VideoBase(
		std::bind(&FFmpegClipImpl::videoModeCallback, std::ref(**this), std::placeholders::_1, std::placeholders::_2) )
	, ClipBase(
		Duration::max(), 
		Duration(),
		std::bind(&FFmpegClip::update, std::ref(*this)) )
	, Signal::SourceLayout<Video>((*this)->videoOut.getOutput())
{
	//Add the output
	ZuazoBase::registerPad((*this)->videoOut.getOutput());

	//Setup the compatibility callback
	(*this)->videoUploader.setVideoModeNegotiationCallback(
		std::bind(&FFmpegClipImpl::videoModeNegotiationCallback, std::ref(**this), std::placeholders::_1, std::placeholders::_2)
	);
}


FFmpegClip::FFmpegClip(FFmpegClip&& other) = default;

FFmpegClip::~FFmpegClip() = default;

FFmpegClip& FFmpegClip::operator=(FFmpegClip&& other) = default;

}
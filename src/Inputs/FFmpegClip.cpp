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
		FFmpegDemuxer&				demuxer;
		int							videoStreamIndex;
		int							audioStreamIndex;
		Processors::FFmpegDecoder 	videoDecoder;
		Processors::FFmpegDecoder 	audioDecoder;
		Processors::FFmpegUploader 	videoUploader;
		TimePoint					lastPTS;

		static constexpr TimePoint NO_PTS = TimePoint(Duration(AV_NOPTS_VALUE));

		Open(Inputs::FFmpegDemuxer& demux)
			: demuxer(demux)
			, videoStreamIndex(getStreamIndex(demuxer, Zuazo::FFmpeg::MediaType::VIDEO))
			, audioStreamIndex(getStreamIndex(demuxer, Zuazo::FFmpeg::MediaType::AUDIO))
			, videoDecoder(demuxer.getInstance(), "Video Decoder", getCodecParameters(demuxer, videoStreamIndex))
			, audioDecoder(demuxer.getInstance(), "Audio Decoder", getCodecParameters(demuxer, audioStreamIndex))
			, videoUploader(demuxer.getInstance(), "Video Uploader")
			, lastPTS(NO_PTS)
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

		void update() {
			const auto lastIndex = demuxer.getLastStreamIndex();

			if(isValidIndex(lastIndex)) {
				if(lastIndex == videoStreamIndex) {
					update(videoDecoder, videoStreamIndex);
				} else if (lastIndex == audioStreamIndex) {
					//update(audioDecoder, audioStreamIndex); //TODO currently no need to decode audio
				}
			}
		}

		void flush() {
			flush(videoDecoder, videoStreamIndex);
			flush(audioDecoder, audioStreamIndex);
			lastPTS = NO_PTS;
		}

	private:
		void update(Processors::FFmpegDecoder& decoder, int index) {
			assert(decoder.isOpen());
			if(isValidIndex(index)) {
				decoder.update();
				
				//Update the timestamp
				const auto timeStamp = decoder.getLastPTS();
				if(timeStamp != AV_NOPTS_VALUE) {
					const auto streams = demuxer.getStreams();
					const auto srcTimeBase = streams[index].getTimeBase();
					const auto dstTimeBase = Math::Rational<int>(Duration::period::num, Duration::period::den);
					const auto rescaledTimeStamp =  av_rescale_q(
						timeStamp, 
						AVRational{ srcTimeBase.getNumerator(), srcTimeBase.getDenominator() },
						AVRational{ dstTimeBase.getNumerator(), dstTimeBase.getDenominator() }
					);

					lastPTS = Math::max(lastPTS, TimePoint(Duration(rescaledTimeStamp)));
				}
			}
		}

		static bool isValidIndex(int index) {
			return index >= 0;
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
		auto& ff = static_cast<FFmpegClip&>(base);
		assert(&owner.get() == &ff);

		demuxer.open();
		opened = Utils::makeUnique<Open>(demuxer);

		//Route the output signal
		videoOut << Signal::getOutput<Zuazo::Video>(opened->videoUploader);

		//ff.setDuration(Duration()); //TODO
		//ff.setTimeStep(Duration()); //TODO
		ff.setTime(ff.getTime()); //Ensure time point is within limits
		ff.enableRegularUpdate(Instance::INPUT_PRIORITY);

		refresh(ff); //Ensure that the first frame has been decoded
	}

	void close(ZuazoBase& base) {
		assert(opened);
		auto& ff = static_cast<FFmpegClip&>(base);
		assert(&owner.get() == &ff);

		ff.disableRegularUpdate();
		ff.setDuration(Duration::max());
		ff.setTimeStep(Duration());


		opened.reset();
		demuxer.close();
	}

	void update() {
		FFmpegClip& ff = owner;
		const auto delta = ff.getInstance().getDeltaT();
		ff.advance(delta);
	}

	void refresh(ClipBase& base) {
		if(opened) {
			const auto timePoint = base.getTime();
			const auto delta = timePoint - opened->lastPTS;

			if(delta < Duration(0)) {
				//Time has gone back!

				demuxer.seek(timePoint);
				demuxer.flush();
				opened->flush();
			}

			while(opened->lastPTS < timePoint) {
				demuxer.update();
				opened->update();
			}
		}
	}
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
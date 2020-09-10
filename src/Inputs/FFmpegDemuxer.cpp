#include <zuazo/Inputs/FFmpegDemuxer.h>

#include "../FFmpeg/InputFormatContext.h"


#include <zuazo/Utils/Functions.h>
#include <zuazo/Utils/Pool.h>
#include <zuazo/Signal/Output.h>
#include <zuazo/FFmpeg/Signals.h>
#include <zuazo/FFmpeg/FFmpegConversions.h>

#include <memory>
#include <cassert>
#include <vector>

extern "C" {
	#include <libavutil/avutil.h>
	#include <libavutil/mathematics.h>
}


namespace Zuazo::Inputs {

/*
 * FFmpegDemuxer::Impl
 */

struct FFmpegDemuxer::Impl {
	struct Open {

		using PacketPool = Utils::Pool<FFmpeg::Packet>;
		using Output = Signal::Output<FFmpeg::PacketStream>;

		FFmpeg::InputFormatContext 	formatContext;
		PacketPool 					pool;
		std::vector<Output> 		pads;
		int							lastIndex;


		Open(const char* url) 
			: formatContext(url)
			, pool()
			, pads(createPads(formatContext))
			, lastIndex(-1)
		{
		}

		~Open() = default;

		void update() {
			//Acuqire a frame from the pool for demuxing
			auto packet = pool.acquire();
			assert(packet);

			//Ensure that the frame is clear in order to avoid sending garbage
			packet->unref();

			const int readResult = formatContext.readPacket(*packet);
			switch(readResult) {
			case 0:	//Success!
				lastIndex = packet->getStreamIndex(); //Succesfully extracted a frame
				break;
			case AVERROR_EOF: //End of file: Signal flusing mode (packet will be empty)
				lastIndex = (lastIndex + 1) % pads.size(); //Just walk though all the pads
				break;
			default: //Unexpected!
				lastIndex = -1;
				return;

			}

			assert(lastIndex >= 0 && lastIndex < static_cast<int>(pads.size()));
			pads[lastIndex].push(std::move(packet));
		}

	private:
		static std::vector<Output> createPads(const FFmpeg::InputFormatContext& fmt) {
			const size_t streamCount = fmt.getStreams().size();
			std::vector<Output> result;
			result.reserve(streamCount);

			for(size_t i = 0; i < streamCount; i++) {
				result.emplace_back(Signal::makeOutputName<FFmpeg::PacketStream>(i));
			}

			return result;
		}

	};

	std::string 			url;
	std::unique_ptr<Open> 	opened;

	Impl(std::string url) 
		: url(std::move(url))
	{
	}

	~Impl() = default;


	void open(ZuazoBase& base) {
		assert(!opened);
		auto& demux = static_cast<FFmpegDemuxer&>(base);

		opened = Utils::makeUnique<Open>(url.c_str());

		for(auto& pad : opened->pads) {
			demux.registerPad(pad);
		}
	}

	void close(ZuazoBase& base) {
		assert(opened);
		auto& demux = static_cast<FFmpegDemuxer&>(base);

		for(auto& pad : opened->pads) {
			demux.removePad(pad);
		}

		opened.reset();
	}

	void update() {
		if(opened) opened->update();
	}



	Streams getStreams() const {
		return opened
		? opened->formatContext.getStreams()
		: Streams();
	}

	int findBestStream(FFmpeg::MediaType type) const {
		return opened
		? opened->formatContext.findBestStream(type)
		: -1;
	}
	
	int getLastStreamIndex() const {
		return opened 
		? opened->lastIndex
		: -1;
	}

	Duration getDuration() const {
		return opened 
		? Duration(av_rescale_q(opened->formatContext.getDuration(), AV_TIME_BASE_Q, AVRational{Duration::period::num, Duration::period::den}))
		: Duration();
	}



	bool seek(int stream, int64_t timestamp, FFmpeg::SeekFlags flags) {
		return opened 
		? opened->formatContext.seek(stream, timestamp, flags) >= 0
		: false;
	}

	bool seek(TimePoint tp, FFmpeg::SeekFlags flags) {
		return opened 
		? opened->formatContext.seek(av_rescale_q(tp.time_since_epoch().count(), AVRational{Duration::period::num, Duration::period::den}, AV_TIME_BASE_Q), flags) >= 0
		: false;
	}
	
	bool flush() {
		return opened 
		? opened->formatContext.flush() >= 0
		: false;
	}
};



/*
 * FFmpegDemuxer
 */

FFmpegDemuxer::FFmpegDemuxer(Instance& instance, std::string name, std::string url)
	: ZuazoBase(instance, std::move(name))
	, m_impl({}, std::move(url))
{
	setOpenCallback(std::bind(&Impl::open, std::ref(*m_impl), std::placeholders::_1));
	setCloseCallback(std::bind(&Impl::close, std::ref(*m_impl), std::placeholders::_1));
	setUpdateCallback(std::bind(&Impl::update, std::ref(*m_impl)));
}

FFmpegDemuxer::FFmpegDemuxer(FFmpegDemuxer&& other) = default;

FFmpegDemuxer::~FFmpegDemuxer() = default;

FFmpegDemuxer& FFmpegDemuxer::operator=(FFmpegDemuxer&& other) = default;



FFmpegDemuxer::Streams FFmpegDemuxer::getStreams() const {
	return m_impl->getStreams();
}

int FFmpegDemuxer::findBestStream(FFmpeg::MediaType type) const {
	return m_impl->findBestStream(type);
}

int FFmpegDemuxer::getLastStreamIndex() const {
	return m_impl->getLastStreamIndex();
}

Duration FFmpegDemuxer::getDuration() const {
	return m_impl->getDuration();
}


bool FFmpegDemuxer::seek(int stream, int64_t timestamp, FFmpeg::SeekFlags flags) {
	return m_impl->seek(stream, timestamp, flags);
}

bool FFmpegDemuxer::seek(TimePoint tp, FFmpeg::SeekFlags flags) {
	return m_impl->seek(tp, flags);
}

bool FFmpegDemuxer::flush() {
	return m_impl->flush();
}


}
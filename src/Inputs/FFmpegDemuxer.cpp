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
			//packet->unref(); //done by readPacket()

			if(formatContext.readPacket(*packet) != 0) {
				return; //Error
			}

			lastIndex = packet->getStreamIndex();
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



	bool seek(int stream, int64_t timestamp) {
		return opened 
		? opened->formatContext.seek(stream, timestamp) >= 0
		: false;
	}

	bool seek(TimePoint tp) {
		return opened 
		? opened->formatContext.seek(FFmpeg::toFFmpeg(tp.time_since_epoch())) >= 0
		: false;
	}
	
	bool seekAny(int stream, int64_t timestamp) {
		return opened 
		? opened->formatContext.seekAny(stream, timestamp)  >= 0
		: false;
	}
	
	bool seekAny(TimePoint tp) {
		return opened 
		? opened->formatContext.seekAny(FFmpeg::toFFmpeg(tp.time_since_epoch())) >= 0
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



bool FFmpegDemuxer::seek(int stream, int64_t timestamp) {
	return m_impl->seek(stream, timestamp);
}

bool FFmpegDemuxer::seek(TimePoint tp) {
	return m_impl->seek(tp);
}

bool FFmpegDemuxer::seekAny(int stream, int64_t timestamp) {
	return m_impl->seekAny(stream, timestamp);
}

bool FFmpegDemuxer::seekAny(TimePoint tp) {
	return m_impl->seek(tp);
}

bool FFmpegDemuxer::flush() {
	return m_impl->flush();
}


}
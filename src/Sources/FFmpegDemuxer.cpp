#include <zuazo/Sources/FFmpegDemuxer.h>

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


namespace Zuazo::Sources {

/*
 * FFmpegDemuxerImpl
 */

struct FFmpegDemuxerImpl {
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

	FFmpegDemuxerImpl(std::string url) 
		: url(std::move(url))
	{
	}

	~FFmpegDemuxerImpl() = default;


	void open(ZuazoBase& base) {
		auto& demux = static_cast<FFmpegDemuxer&>(base);
		assert(!opened);

		opened = Utils::makeUnique<Open>(url.c_str()); //May throw! (nothing has been done yet, so don't worry about cleaning)

		assert(opened);
		for(auto& pad : opened->pads) {
			demux.registerPad(pad);
		}

		assert(opened);
	}

	void asyncOpen(ZuazoBase& base, std::unique_lock<Instance>& lock) {
		auto& demux = static_cast<FFmpegDemuxer&>(base);
		assert(!opened);
		assert(lock.owns_lock());

		lock.unlock(); //FIXME, if it throws, lock must be re-locked
		auto newOpened = Utils::makeUnique<Open>(url.c_str()); //May throw! (nothing has been done yet, so don't worry about cleaning)
		lock.lock();
		
		opened = std::move(newOpened);
		for(auto& pad : opened->pads) {
			demux.registerPad(pad);
		}

		assert(opened);
		assert(lock.owns_lock());
	}

	void close(ZuazoBase& base) {
		auto& demux = static_cast<FFmpegDemuxer&>(base);
		assert(opened);

		for(auto& pad : opened->pads) {
			demux.removePad(pad);
		}
		
		opened.reset();

		assert(!opened);
	}

	void asyncClose(ZuazoBase& base, std::unique_lock<Instance>& lock) {
		auto& demux = static_cast<FFmpegDemuxer&>(base);
		assert(opened);
		assert(lock.owns_lock());

		for(auto& pad : opened->pads) {
			demux.removePad(pad);
		}
		auto oldOpened = std::move(opened);

		lock.unlock();
		oldOpened.reset();
		lock.lock();

		assert(!opened);
		assert(lock.owns_lock());
	}

	void update() {
		if(opened) {
			opened->update();
		}
	}



	FFmpegDemuxer::Streams getStreams() const {
		return opened
		? opened->formatContext.getStreams()
		: FFmpegDemuxer::Streams();
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

	FFmpeg::Duration getDuration() const {
		return opened 
		? opened->formatContext.getDuration()
		: FFmpeg::Duration();
	}



	bool seek(int stream, int64_t timestamp, FFmpeg::SeekFlags flags) {
		return opened 
		? opened->formatContext.seek(stream, timestamp, flags) >= 0
		: false;
	}

	bool seek(FFmpeg::Duration timestamp, FFmpeg::SeekFlags flags) {
		return opened 
		? opened->formatContext.seek(timestamp, flags) >= 0
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
	: Utils::Pimpl<FFmpegDemuxerImpl>({}, std::move(url))
	, ZuazoBase(
		instance, 
		std::move(name),
		{},
		ZuazoBase::MoveCallback(),
		std::bind(&FFmpegDemuxerImpl::open, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegDemuxerImpl::asyncOpen, std::ref(**this), std::placeholders::_1, std::placeholders::_2),
		std::bind(&FFmpegDemuxerImpl::close, std::ref(**this), std::placeholders::_1),
		std::bind(&FFmpegDemuxerImpl::asyncClose, std::ref(**this), std::placeholders::_1, std::placeholders::_2),
		std::bind(&FFmpegDemuxerImpl::update, std::ref(**this)) )
{
}

FFmpegDemuxer::FFmpegDemuxer(FFmpegDemuxer&& other) = default;

FFmpegDemuxer::~FFmpegDemuxer() = default;

FFmpegDemuxer& FFmpegDemuxer::operator=(FFmpegDemuxer&& other) = default;



FFmpegDemuxer::Streams FFmpegDemuxer::getStreams() const {
	return (*this)->getStreams();
}

int FFmpegDemuxer::findBestStream(FFmpeg::MediaType type) const {
	return (*this)->findBestStream(type);
}

int FFmpegDemuxer::getLastStreamIndex() const {
	return (*this)->getLastStreamIndex();
}

FFmpeg::Duration FFmpegDemuxer::getDuration() const {
	return (*this)->getDuration();
}


bool FFmpegDemuxer::seek(int stream, int64_t timestamp, FFmpeg::SeekFlags flags) {
	return (*this)->seek(stream, timestamp, flags);
}

bool FFmpegDemuxer::seek(FFmpeg::Duration tp, FFmpeg::SeekFlags flags) {
	return (*this)->seek(tp, flags);
}

bool FFmpegDemuxer::flush() {
	return (*this)->flush();
}


}
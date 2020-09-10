#pragma once

#include "../FFmpeg/Enumerations.h"
#include "../FFmpeg/StreamParameters.h"

#include <zuazo/ZuazoBase.h>
#include <zuazo/Utils/Pimpl.h>
#include <zuazo/Chrono.h>

#include <string>

namespace Zuazo::Inputs {

class FFmpegDemuxer
	: public ZuazoBase
{
public:
	using Streams = Utils::BufferView<const FFmpeg::StreamParameters>;

	FFmpegDemuxer(Instance& instance, std::string name, std::string url = "");
	FFmpegDemuxer(const FFmpegDemuxer& other) = delete;
	FFmpegDemuxer(FFmpegDemuxer&& other);
	~FFmpegDemuxer();

	FFmpegDemuxer&			operator=(const FFmpegDemuxer& other) = delete;
	FFmpegDemuxer&			operator=(FFmpegDemuxer&& other);

	using ZuazoBase::update;

	Streams					getStreams() const;
	int						findBestStream(FFmpeg::MediaType type) const;
	int						getLastStreamIndex() const;
	Duration 				getDuration() const;

	bool					seek(int stream, int64_t timestamp);
	bool					seek(TimePoint tp);
	bool					seekAny(int stream, int64_t timestamp);
	bool					seekAny(TimePoint tp);
	bool					flush();
private:
	struct Impl;
	Utils::Pimpl<Impl>		m_impl;

};

}
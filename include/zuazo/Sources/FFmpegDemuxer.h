#pragma once

#include "../FFmpeg/Enumerations.h"
#include "../FFmpeg/Chrono.h"
#include "../FFmpeg/StreamParameters.h"

#include <zuazo/ZuazoBase.h>
#include <zuazo/Utils/Pimpl.h>
#include <zuazo/Chrono.h>

#include <string>

namespace Zuazo::Sources {

class FFmpegDemuxer
	: public Utils::Pimpl<struct FFmpegDemuxerImpl>
	, public ZuazoBase
{
	friend FFmpegDemuxerImpl;
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
	FFmpeg::Duration		getDuration() const;

	bool					seek(int stream, int64_t timestamp, FFmpeg::SeekFlags flags = FFmpeg::SeekFlags::NONE);
	bool					seek(FFmpeg::Duration timestamp, FFmpeg::SeekFlags flags = FFmpeg::SeekFlags::NONE);
	bool					flush();

};

}
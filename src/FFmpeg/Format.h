#pragma once

#include <zuazo/FFmpeg/Packet.h>
#include <zuazo/FFmpeg/MediaType.h>

#include <zuazo/Utils/BufferView.h>

#include <cstddef>

extern "C" {
	#include <libavformat/avformat.h>
}

namespace Zuazo::FFmpeg {

class FormatContext {
public:
	using Handle = AVFormatContext*;
	using ConstHandle = const AVFormatContext*;

	FormatContext(const char* url);
	FormatContext(const FormatContext& other) = delete;
	FormatContext(FormatContext&& other);
	~FormatContext();

	FormatContext& 						operator=(const FormatContext& other) = delete;
	FormatContext&						operator=(FormatContext&& other);

	operator Handle();
	operator ConstHandle() const;

	void								swap(FormatContext& other);

	int									writeHeader();

	int									findBestStream(MediaType type) const;

	int									play();
	int									pause();
	int									seek(int stream, int64_t timestamp);
	int									seekAny(int stream, int64_t timestamp);
	int									readPacket(Packet& pkt);
	int									writePacket(Packet& pkt);
	int									flush();

private:
	Handle								m_handle;
	
};

}
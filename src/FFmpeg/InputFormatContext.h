#pragma once

#include <zuazo/FFmpeg/Packet.h>
#include <zuazo/FFmpeg/StreamParameters.h>
#include <zuazo/FFmpeg/Enumerations.h>
#include <zuazo/FFmpeg/Chrono.h>

#include <zuazo/Utils/BufferView.h>

#include <cstddef>
#include <chrono>

struct AVFormatContext;

namespace Zuazo::FFmpeg {

class InputFormatContext {
public:
	using Handle = AVFormatContext*;
	using ConstHandle = const AVFormatContext*;

	using Streams = Utils::BufferView<const StreamParameters>;

	InputFormatContext();
	InputFormatContext(const char* url);
	InputFormatContext(const InputFormatContext& other) = delete;
	InputFormatContext(InputFormatContext&& other);
	~InputFormatContext();

	InputFormatContext& 				operator=(const InputFormatContext& other) = delete;
	InputFormatContext&					operator=(InputFormatContext&& other);

	operator Handle();
	operator ConstHandle() const;

	void								swap(InputFormatContext& other);

	Streams 							getStreams() const;
	int									findBestStream(MediaType type) const;
	
	Duration							getDuration() const;

	int									play();
	int									pause();
	int									seek(int stream, int64_t timestamp, SeekFlags flags = SeekFlags::none);
	int									seek(Duration timestamp, SeekFlags flags = SeekFlags::none);
	int									readPacket(Packet& pkt);
	int									flush();

private:
	Handle								m_handle;

	AVFormatContext&					get();
	const AVFormatContext&				get() const;
	
};

}
#pragma once

#include <zuazo/FFmpeg/Packet.h>
#include <zuazo/FFmpeg/StreamParameters.h>
#include <zuazo/FFmpeg/Enumerations.h>

#include <zuazo/Utils/BufferView.h>

#include <cstddef>

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

	int									play();
	int									pause();
	int									seek(int stream, int64_t timestamp);
	int									seek(int64_t timestamp);
	int									seekAny(int stream, int64_t timestamp);
	int									seekAny(int64_t timestamp);
	int									readPacket(Packet& pkt);
	int									flush();

private:
	Handle								m_handle;

	AVFormatContext&					get();
	const AVFormatContext&				get() const;
	
};

}
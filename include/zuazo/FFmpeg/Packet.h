#pragma once

#include <zuazo/Utils/BufferView.h>

#include <cstddef>

struct AVPacket;

namespace Zuazo::FFmpeg {

class Packet {
public:
	using Handle = AVPacket*;
	using ConstHandle = const AVPacket*;

	Packet();
	Packet(size_t size);
	Packet(ConstHandle hand);
	Packet(const Packet& other);
	Packet(Packet&& other);
	~Packet();

	Packet& 							operator=(const Packet& other);
	Packet&								operator=(Packet&& other);

	operator Handle();
	operator ConstHandle() const;

	void								swap(Packet& other);

	void								setPTS(int64_t pts);
	int64_t								getPTS() const;

	void								setDTS(int64_t dts);
	int64_t								getDTS() const;

	void								setDuration(int64_t duration);
	int64_t								getDuration() const;
	
	void								setStreamIndex(int idx);
	int									getStreamIndex() const;

	Utils::BufferView<std::byte> 		getData();
	Utils::BufferView<const std::byte>	getData() const;

	void								shrink(size_t size);
	void								grow(size_t size);

private:
	Handle								m_handle;
	
};

}
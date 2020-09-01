#pragma once

#include <zuazo/FFmpeg/CodecID.h>

#include <cstddef>

struct AVCodec;

namespace Zuazo::FFmpeg {

class Codec {
public:
	using Handle = AVCodec*;
	using ConstHandle = const AVCodec*;

	Codec(Handle handle);
	Codec(const Codec& other) = default;
	~Codec() = default;

	Codec& 						operator=(const Codec& other) = default;

	operator Handle();
	operator ConstHandle() const;

private:
	Handle						m_handle;
	
};

class Encoder : public Codec {
public:
	Encoder(CodecId codecId);
	Encoder(const char* name);
	Encoder(const Encoder& other) = default;
	~Encoder() = default;

	Encoder& 					operator=(const Encoder& other) = default;

};

class Decoder : public Codec {
public:
	Decoder(CodecId codecId);
	Decoder(const char* name);
	Decoder(const Decoder& other) = default;
	~Decoder() = default;

	Decoder& 					operator=(const Decoder& other) = default;

};

}
#pragma once

#include "Codec.h"
#include "Frame.h"

#include <zuazo/FFmpeg/Packet.h>
#include <zuazo/FFmpeg/CodecID.h>
#include <zuazo/FFmpeg/MediaType.h>

#include <zuazo/Utils/BufferView.h>

#include <cstddef>

extern "C" {
	#include <libavcodec/avcodec.h>
}

namespace Zuazo::FFmpeg {

class CodecContext {
public:
	using Handle = AVCodecContext*;
	using ConstHandle = const AVCodecContext*;

	CodecContext(Codec codec);
	CodecContext(const CodecContext& other) = delete;
	CodecContext(CodecContext&& other);
	~CodecContext();

	CodecContext& 						operator=(const CodecContext& other) = delete;
	CodecContext&						operator=(CodecContext&& other);

	operator Handle();
	operator ConstHandle() const;

	void								swap(CodecContext& other);

	void								open(const Codec& codec);

	void								setParameters(const AVCodecParameters& parameters);
	void								getParameters(AVCodecParameters& parameters) const;
	AVCodecParameters					getParameters() const;

	int									sendPacket(const Packet& packet);
	int									readPacket(Packet& packet);
	int									sendFrame(const Frame& frame);
	int									readFrame(Frame& frame);


private:
	Handle								m_handle;
	
};

}
#pragma once

#include <zuazo/FFmpeg/Packet.h>
#include <zuazo/FFmpeg/Frame.h>
#include <zuazo/FFmpeg/Enumerations.h>
#include <zuazo/FFmpeg/CodecParameters.h>

#include <zuazo/Utils/BufferView.h>

#include <cstddef>

struct AVCodec;
struct AVCodecContext;

namespace Zuazo::FFmpeg {

class CodecContext {
public:
	using Handle = AVCodecContext*;
	using ConstHandle = const AVCodecContext*;

	using PixelFormatNegotiationCallback = PixelFormat (*)(Handle, const PixelFormat *fmt);

	CodecContext();
	CodecContext(const AVCodec *codec); //TODO creare a Codec class to avoid ffmpeg pointers on the iface
	CodecContext(const CodecContext& other) = delete;
	CodecContext(CodecContext&& other);
	~CodecContext();

	CodecContext& 						operator=(const CodecContext& other) = delete;
	CodecContext&						operator=(CodecContext&& other);

	operator Handle();
	operator ConstHandle() const;

	void								swap(CodecContext& other);

	int									open(const AVCodec* codec = nullptr);

	void								setOpaque(void* userData);
	void*								getOpaque() const;

	int									setParameters(const CodecParameters& parameters);
	int									getParameters(CodecParameters& parameters) const;
	CodecParameters						getParameters() const;

	void								setPixelFormatNegotiationCallback(PixelFormatNegotiationCallback cbk);
	PixelFormatNegotiationCallback		getPixelFormatNegotiationCallback() const;

	void								setThreadCount(int cnt);
	int									getThreadCount() const;

	void								setThreadType(ThreadType type);
	ThreadType							getThreadType() const;

	int									sendPacket(const Packet& packet);
	int									readPacket(Packet& packet);
	int									sendFrame(const Frame& frame);
	int									readFrame(Frame& frame);

	void								flush();

private:
	Handle								m_handle;
	
	AVCodecContext&						get();
	const AVCodecContext&				get() const;
};

}
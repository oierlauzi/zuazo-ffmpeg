#pragma once

#include <zuazo/ZuazoBase.h>
#include <zuazo/Signal/ProcessorLayout.h>
#include <zuazo/Utils/Pimpl.h>
#include <zuazo/FFmpeg/Signals.h>
#include <zuazo/FFmpeg/Enumerations.h>
#include <zuazo/FFmpeg/CodecParameters.h>

#include <functional>

namespace Zuazo::Processors {

class FFmpegDecoder
	: public Utils::Pimpl<struct FFmpegDecoderImpl>
	, public ZuazoBase
	, public Signal::ProcessorLayout<FFmpeg::PacketStream, FFmpeg::FrameStream>
{
	friend FFmpegDecoderImpl;
public:
	using PixelFormatNegotiationCallback = std::function<FFmpeg::PixelFormat(FFmpegDecoder&, const FFmpeg::PixelFormat*)>;
	using DemuxCallback = std::function<void()>;

	FFmpegDecoder(	Instance& instance, 
					std::string name, 
					FFmpeg::CodecParameters codecPar = {},
					PixelFormatNegotiationCallback pixFmtCbk = {},
					DemuxCallback demuxCbk = {});
	FFmpegDecoder(const FFmpegDecoder& other) = delete;
	FFmpegDecoder(FFmpegDecoder&& other);
	~FFmpegDecoder();

	FFmpegDecoder&					operator=(const FFmpegDecoder& other) = delete;
	FFmpegDecoder&					operator=(FFmpegDecoder&& other);

	using ZuazoBase::update;

	void							readPacket();
	void							flush();

	void							setCodecParameters(FFmpeg::CodecParameters codecPar);
	const FFmpeg::CodecParameters& 	getCodecParameters() const;	

	FFmpeg::HWDeviceType			getHardwareDeviceType() const;

	void							setThreadType(FFmpeg::ThreadType type);
	FFmpeg::ThreadType				getThreadType() const;

	void							setThreadCount(int cnt);
	int								getThreadCount() const;	

	void							setPixelFormatNegotiationCallback(PixelFormatNegotiationCallback cbk);
	const PixelFormatNegotiationCallback& getPixelFormatNegotiationCallback() const;

	void							setDemuxCallback(DemuxCallback cbk);
	const DemuxCallback&			getDemuxCallback() const;

};

}
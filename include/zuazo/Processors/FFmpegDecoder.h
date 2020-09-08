#pragma once

#include <zuazo/ZuazoBase.h>
#include <zuazo/Utils/Pimpl.h>
#include <zuazo/FFmpeg/Enumerations.h>
#include <zuazo/FFmpeg/CodecParameters.h>

namespace Zuazo::Processors {

class FFmpegDecoder
	: public ZuazoBase
{
public:
	FFmpegDecoder(Instance& instance, std::string name, FFmpeg::CodecParameters codecPar = {});
	FFmpegDecoder(const FFmpegDecoder& other) = delete;
	FFmpegDecoder(FFmpegDecoder&& other);
	~FFmpegDecoder();

	FFmpegDecoder&					operator=(const FFmpegDecoder& other) = delete;
	FFmpegDecoder&					operator=(FFmpegDecoder&& other);

	using ZuazoBase::update;

	void							setCodecParameters(FFmpeg::CodecParameters codecPar);
	const FFmpeg::CodecParameters& 	getCodecParameters() const;

	void							setThreadCount(int cnt);
	int								getThreadCount() const;				

	void							setThreadType(FFmpeg::ThreadType type);
	FFmpeg::ThreadType				getThreadType() const;

	int64_t							getLastPTS() const;
	void							flush();

private:
	struct Impl;
	Utils::Pimpl<Impl>		m_impl;

};

}
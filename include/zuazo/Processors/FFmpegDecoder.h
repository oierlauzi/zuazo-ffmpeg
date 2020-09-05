#pragma once

#include <zuazo/ZuazoBase.h>
#include <zuazo/Utils/Pimpl.h>
#include <zuazo/Chrono.h>
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

	FFmpegDecoder&			operator=(const FFmpegDecoder& other) = delete;
	FFmpegDecoder&			operator=(FFmpegDecoder&& other);

	using ZuazoBase::update;


private:
	struct Impl;
	Utils::Pimpl<Impl>		m_impl;

};

}
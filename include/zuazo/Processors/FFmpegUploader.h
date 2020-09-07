#pragma once

#include <zuazo/ZuazoBase.h>
#include <zuazo/Utils/Pimpl.h>
#include <zuazo/Video.h>
#include <zuazo/FFmpeg/CodecParameters.h>

namespace Zuazo::Processors {

class FFmpegUploader
	: public ZuazoBase
{
public:
	FFmpegUploader(Instance& instance, std::string name);
	FFmpegUploader(const FFmpegUploader& other) = delete;
	FFmpegUploader(FFmpegUploader&& other);
	~FFmpegUploader();

	FFmpegUploader&			operator=(const FFmpegUploader& other) = delete;
	FFmpegUploader&			operator=(FFmpegUploader&& other);

private:
	struct Impl;
	Utils::Pimpl<Impl>		m_impl;

};

}
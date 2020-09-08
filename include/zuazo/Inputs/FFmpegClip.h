#pragma once

#include "../FFmpeg/Enumerations.h"
#include "../FFmpeg/StreamParameters.h"

#include <zuazo/ZuazoBase.h>
#include <zuazo/Video.h>
#include <zuazo/ClipBase.h>
#include <zuazo/Utils/Pimpl.h>

namespace Zuazo::Inputs {

class FFmpegClip
	: public ZuazoBase
	, public VideoBase
	, public ClipBase
{
public:
	FFmpegClip(	Instance& instance, 
			std::string name, 
			VideoMode videoMode = VideoMode::ANY,
			std::string url = "" );

	FFmpegClip(const FFmpegClip& other) = delete;
	FFmpegClip(FFmpegClip&& other);
	~FFmpegClip();

	FFmpegClip& 			operator=(const FFmpegClip& other) = delete;
	FFmpegClip& 			operator=(FFmpegClip&& other);

private:
	struct Impl;
	Utils::Pimpl<Impl>		m_impl;
};
	
}
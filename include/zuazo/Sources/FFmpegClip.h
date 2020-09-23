#pragma once

#include "../FFmpeg/Enumerations.h"
#include "../FFmpeg/StreamParameters.h"

#include <zuazo/ZuazoBase.h>
#include <zuazo/Video.h>
#include <zuazo/ClipBase.h>
#include <zuazo/Signal/SourceLayout.h>
#include <zuazo/Utils/Pimpl.h>

namespace Zuazo::Sources {

class FFmpegClip
	: public Utils::Pimpl<struct FFmpegClipImpl>
	, public ZuazoBase
	, public VideoBase
	, public ClipBase
	, public Signal::SourceLayout<Video>
{
	friend FFmpegClipImpl;
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

};
	
}
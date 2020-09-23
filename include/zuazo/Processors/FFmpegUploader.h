#pragma once

#include <zuazo/ZuazoBase.h>
#include <zuazo/Video.h>
#include <zuazo/Signal/ProcessorLayout.h>
#include <zuazo/Utils/Pimpl.h>
#include <zuazo/FFmpeg/Signals.h>
#include <zuazo/FFmpeg/CodecParameters.h>

namespace Zuazo::Processors {

class FFmpegUploader
	: public Utils::Pimpl<struct FFmpegUploaderImpl>
	, public ZuazoBase
	, public VideoBase
	, public Signal::ProcessorLayout<FFmpeg::FrameStream, Video>
{
	friend FFmpegUploaderImpl;
public:
	FFmpegUploader(	Instance& instance, 
					std::string name, 
					VideoMode videoMode = VideoMode::ANY);
	FFmpegUploader(const FFmpegUploader& other) = delete;
	FFmpegUploader(FFmpegUploader&& other);
	~FFmpegUploader();

	FFmpegUploader&			operator=(const FFmpegUploader& other) = delete;
	FFmpegUploader&			operator=(FFmpegUploader&& other);

};

}
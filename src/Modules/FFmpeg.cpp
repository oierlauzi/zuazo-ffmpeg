#include <zuazo/Modules/FFmpeg.h>

#include <cassert>

namespace Zuazo::Modules {

std::unique_ptr<FFmpeg> FFmpeg::s_singleton;

FFmpeg::FFmpeg() 
	: Instance::Module(std::string(name), version)
{
}

FFmpeg::~FFmpeg() = default;


const FFmpeg& FFmpeg::get() {
	if(!s_singleton) {
		s_singleton = std::unique_ptr<FFmpeg>(new FFmpeg);
	}

	assert(s_singleton);
	return *s_singleton;
}

}
#pragma once

#include <zuazo/Instance.h>

#include <memory>

namespace Zuazo::Modules {

class FFmpeg final
	: public Instance::Module
{
public:
	~FFmpeg();

	static constexpr std::string_view name = "FFmpeg";
	static constexpr Version version = Version(0, 1, 0);

	static const FFmpeg& 				get();

private:
	FFmpeg();
	FFmpeg(const FFmpeg& other) = delete;

	FFmpeg& 							operator=(const FFmpeg& other) = delete;

	static std::unique_ptr<FFmpeg> 		s_singleton;
};

}
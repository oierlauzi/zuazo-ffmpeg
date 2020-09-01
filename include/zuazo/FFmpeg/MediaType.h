#pragma once

#include <string_view>

namespace Zuazo::FFmpeg {

enum class MediaType {
	NONE,

	VIDEO,
	AUDIO,
	DATA,
	SUBTITLE,
	ATTACHMENT,

	COUNT
};

}

namespace Zuazo {

constexpr std::string_view toString(FFmpeg::MediaType type);
std::ostream& operator<<(std::ostream& os, FFmpeg::MediaType type);


}

#include "MediaType.inl"
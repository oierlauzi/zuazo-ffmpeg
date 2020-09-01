#pragma once

#include "MediaType.h"

#include <string_view>

namespace Zuazo::FFmpeg {

enum class CodecId {};

MediaType getMediaType(FFmpeg::CodecId id);

}

namespace Zuazo {

std::string_view toString(FFmpeg::CodecId id);
std::ostream& operator<<(std::ostream& os, FFmpeg::CodecId id);

}
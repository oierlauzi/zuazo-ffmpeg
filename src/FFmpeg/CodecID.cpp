#include <zuazo/FFmpeg/CodecID.h>

#include "FFmpegConversions.h"

extern "C" {
	#include <libavcodec/codec_id.h>
}

namespace Zuazo::FFmpeg {

MediaType getMediaType(FFmpeg::CodecId id) {
	return fromFFmpeg(avcodec_get_type(toFFmpeg(id)));
}

}

namespace Zuazo {

std::string_view toString(FFmpeg::CodecId id) {
	return std::string_view(avcodec_get_name(toFFmpeg(id)));
}

std::ostream& operator<<(std::ostream& os, FFmpeg::CodecId id) {
	return os << toString(id);
}

}
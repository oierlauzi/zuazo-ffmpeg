#include <zuazo/FFmpeg/Enumerations.h>

extern "C" {
	#include <libavutil/avutil.h>
	#include <libavutil/pixfmt.h>
	#include <libavutil/pixdesc.h>
	#include <libavcodec/codec_id.h>
	#include <libavcodec/avcodec.h>
}

namespace Zuazo::FFmpeg {

static_assert(static_cast<AVMediaType>(MediaType::NONE) == AVMEDIA_TYPE_UNKNOWN, "MediaType null value must match");
static_assert(static_cast<AVMediaType>(MediaType::VIDEO) == AVMEDIA_TYPE_VIDEO, "MediaType VIDEO value must match");
static_assert(static_cast<AVMediaType>(MediaType::AUDIO) == AVMEDIA_TYPE_AUDIO, "MediaType AUDIO value must match");
static_assert(static_cast<AVMediaType>(MediaType::DATA) == AVMEDIA_TYPE_DATA, "MediaType DATA value must match");
static_assert(static_cast<AVMediaType>(MediaType::SUBTITLE) == AVMEDIA_TYPE_SUBTITLE, "MediaType SUBTITLE value must match");
static_assert(static_cast<AVMediaType>(MediaType::ATTACHMENT) == AVMEDIA_TYPE_ATTACHMENT, "MediaType ATTACHMENT value must match");

static_assert(static_cast<AVCodecID>(CodecID::NONE) == AV_CODEC_ID_NONE, "CodecID null value must match");

static_assert(static_cast<AVPixelFormat>(PixelFormat::NONE) == AV_PIX_FMT_NONE, "PixelFormat null value must match");

static_assert(static_cast<AVColorPrimaries>(ColorPrimaries::NONE) == AVCOL_PRI_UNSPECIFIED, "ColorPrimaries null value must match");

static_assert(static_cast<AVColorTransferCharacteristic>(ColorTransferCharacteristic::NONE) == AVCOL_TRC_UNSPECIFIED, "ColorTransferFunction null value must match");

static_assert(static_cast<AVColorSpace>(ColorSpace::NONE) == AVCOL_SPC_UNSPECIFIED, "ColorSpace null value must match");

static_assert(static_cast<AVColorRange>(ColorRange::NONE) == AVCOL_RANGE_UNSPECIFIED, "ColorRange null value must match");

static_assert(static_cast<AVChromaLocation>(ChromaLocation::NONE) == AVCHROMA_LOC_UNSPECIFIED, "ColorRange null value must match");

static_assert(static_cast<AVFieldOrder>(FieldOrder::NONE) == AV_FIELD_UNKNOWN, "FieldOrder null value must match");
static_assert(static_cast<AVFieldOrder>(FieldOrder::PROGRESSIVE) == AV_FIELD_PROGRESSIVE, "FieldOrder progressive value must match");

static_assert(static_cast<AVDiscard>(Discard::NONE) == AVDISCARD_NONE, "Discard null value must match");



MediaType getMediaType(FFmpeg::CodecID id) {
	return static_cast<MediaType>(avcodec_get_type(static_cast<AVCodecID>(id)));
}

std::string_view getProfileName(FFmpeg::CodecID id, int profile) {
	return avcodec_profile_name(static_cast<AVCodecID>(id), profile);
}

}

namespace Zuazo {

std::string_view toString(FFmpeg::MediaType type) {
	return std::string_view(av_get_media_type_string(static_cast<AVMediaType>(type)));
}

std::ostream& operator<<(std::ostream& os, FFmpeg::MediaType type) {
	return os << toString(type);
}



std::string_view toString(FFmpeg::CodecID id) {
	return std::string_view(avcodec_get_name(static_cast<AVCodecID>(id)));
}

std::ostream& operator<<(std::ostream& os, FFmpeg::CodecID id) {
	return os << toString(id);
}



std::string_view toString(FFmpeg::PixelFormat fmt) {
	return std::string_view(av_get_pix_fmt_name(static_cast<AVPixelFormat>(fmt)));
}

std::ostream& operator<<(std::ostream& os, FFmpeg::PixelFormat fmt) {
	return os << toString(fmt);
}



std::string_view toString(FFmpeg::ColorPrimaries prim) {
	return std::string_view(av_color_primaries_name(static_cast<AVColorPrimaries>(prim)));
}

std::ostream& operator<<(std::ostream& os, FFmpeg::ColorPrimaries prim) {
	return os << toString(prim);
}



std::string_view toString(FFmpeg::ColorTransferCharacteristic trc) {
	return std::string_view(av_color_transfer_name(static_cast<AVColorTransferCharacteristic>(trc)));
}

std::ostream& operator<<(std::ostream& os, FFmpeg::ColorTransferCharacteristic trc) {
	return os << toString(trc);
}



std::string_view toString(FFmpeg::ColorSpace space) {
	return std::string_view(av_color_space_name(static_cast<AVColorSpace>(space)));
}

std::ostream& operator<<(std::ostream& os, FFmpeg::ColorSpace space) {
	return os << toString(space);
}



std::string_view toString(FFmpeg::ColorRange range) {
	return std::string_view(av_color_range_name(static_cast<AVColorRange>(range)));
}

std::ostream& operator<<(std::ostream& os, FFmpeg::ColorRange range) {
	return os << toString(range);
}



std::string_view toString(FFmpeg::ChromaLocation loc) {
	return std::string_view(av_chroma_location_name(static_cast<AVChromaLocation>(loc)));
}

std::ostream& operator<<(std::ostream& os, FFmpeg::ChromaLocation loc) {
	return os << toString(loc);
}


char toString(FFmpeg::PictureType type) {
	return av_get_picture_type_char(static_cast<AVPictureType>(type));
}

std::ostream& operator<<(std::ostream& os, FFmpeg::PictureType type) {
	char str = toString(type);
	return os << std::string_view(&str, 1);
}

}
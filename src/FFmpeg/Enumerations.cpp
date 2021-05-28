#include <zuazo/FFmpeg/Enumerations.h>

extern "C" {
	#include <libavutil/avutil.h>
	#include <libavutil/pixfmt.h>
	#include <libavutil/pixdesc.h>
	#include <libavutil/hwcontext.h>
	#include <libavformat/avformat.h>
	#include <libavcodec/codec_id.h>
	#include <libavcodec/avcodec.h>
}

namespace Zuazo::FFmpeg {

static_assert(sizeof(AVMediaType) == sizeof(MediaType), "MediaType enum's size does not match");
static_assert(static_cast<AVMediaType>(MediaType::none) == AVMEDIA_TYPE_UNKNOWN, "MediaType null value must match");
static_assert(static_cast<AVMediaType>(MediaType::video) == AVMEDIA_TYPE_VIDEO, "MediaType VIDEO value must match");
static_assert(static_cast<AVMediaType>(MediaType::audio) == AVMEDIA_TYPE_AUDIO, "MediaType AUDIO value must match");
static_assert(static_cast<AVMediaType>(MediaType::data) == AVMEDIA_TYPE_DATA, "MediaType DATA value must match");
static_assert(static_cast<AVMediaType>(MediaType::subtitle) == AVMEDIA_TYPE_SUBTITLE, "MediaType SUBTITLE value must match");
static_assert(static_cast<AVMediaType>(MediaType::attachment) == AVMEDIA_TYPE_ATTACHMENT, "MediaType ATTACHMENT value must match");


static_assert(sizeof(AVCodecID) == sizeof(CodecID), "CodecID enum's size does not match");
static_assert(static_cast<AVCodecID>(CodecID::none) == AV_CODEC_ID_NONE, "CodecID null value must match");

static_assert(sizeof(AVPixelFormat) == sizeof(PixelFormat), "PixelFormat enum's size does not match");
static_assert(static_cast<AVPixelFormat>(PixelFormat::none) == AV_PIX_FMT_NONE, "PixelFormat null value must match");

static_assert(sizeof(AVColorPrimaries) == sizeof(ColorPrimaries), "ColorPrimaries enum's size does not match");
static_assert(static_cast<AVColorPrimaries>(ColorPrimaries::none) == AVCOL_PRI_UNSPECIFIED, "ColorPrimaries null value must match");

static_assert(sizeof(AVColorTransferCharacteristic) == sizeof(ColorTransferCharacteristic), "ColorTransferCharacteristic enum's size does not match");
static_assert(static_cast<AVColorTransferCharacteristic>(ColorTransferCharacteristic::none) == AVCOL_TRC_UNSPECIFIED, "ColorTransferFunction null value must match");

static_assert(sizeof(AVColorSpace) == sizeof(ColorSpace), "ColorSpace enum's size does not match");
static_assert(static_cast<AVColorSpace>(ColorSpace::none) == AVCOL_SPC_UNSPECIFIED, "ColorSpace null value must match");

static_assert(sizeof(AVColorRange) == sizeof(ColorRange), "ColorRange enum's size does not match");
static_assert(static_cast<AVColorRange>(ColorRange::none) == AVCOL_RANGE_UNSPECIFIED, "ColorRange null value must match");

static_assert(sizeof(AVChromaLocation) == sizeof(ChromaLocation), "ChromaLocation enum's size does not match");
static_assert(static_cast<AVChromaLocation>(ChromaLocation::none) == AVCHROMA_LOC_UNSPECIFIED, "ColorRange null value must match");

static_assert(sizeof(AVFieldOrder) == sizeof(FieldOrder), "FieldOrder enum's size does not match");
static_assert(static_cast<AVFieldOrder>(FieldOrder::none) == AV_FIELD_UNKNOWN, "FieldOrder null value must match");
static_assert(static_cast<AVFieldOrder>(FieldOrder::progressive) == AV_FIELD_PROGRESSIVE, "FieldOrder progressive value must match");

static_assert(sizeof(AVPictureType) == sizeof(PictureType), "PictureType enum's size does not match");
static_assert(static_cast<AVPictureType>(PictureType::none) == AV_PICTURE_TYPE_NONE, "PictureType null value must match");

static_assert(sizeof(AVFrameSideDataType) == sizeof(FrameSideDataType), "FrameSideDataType enum's size does not match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::panscan) == AV_FRAME_DATA_PANSCAN, "FrameSideDataType PAN_SCAN value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::A53cc) == AV_FRAME_DATA_A53_CC, "FrameSideDataType A53_CC value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::stereo3D) == AV_FRAME_DATA_STEREO3D, "FrameSideDataType STEREO3D value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::matrixEncoding) == AV_FRAME_DATA_MATRIXENCODING, "FrameSideDataType MATRIXENCODING value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::downmixInfo) == AV_FRAME_DATA_DOWNMIX_INFO, "FrameSideDataType DOWNMIX_INFO value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::replayGain) == AV_FRAME_DATA_REPLAYGAIN, "FrameSideDataType REPLAYGAIN value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::displayMatrix) == AV_FRAME_DATA_DISPLAYMATRIX, "FrameSideDataType DISPLAYMATRIX value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::sfd) == AV_FRAME_DATA_AFD, "FrameSideDataType AFD value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::motionVectors) == AV_FRAME_DATA_MOTION_VECTORS, "FrameSideDataType MOTION_VECTORS value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::skipSamples) == AV_FRAME_DATA_SKIP_SAMPLES, "FrameSideDataType SKIP_SAMPLES value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::audioServiceType) == AV_FRAME_DATA_AUDIO_SERVICE_TYPE, "FrameSideDataType AUDIO_SERVICE_TYPE value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::masteringDisplayMetadata) == AV_FRAME_DATA_MASTERING_DISPLAY_METADATA, "FrameSideDataType MASTERING_DISPLAY_METADATA value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::gopTimecode) == AV_FRAME_DATA_GOP_TIMECODE, "FrameSideDataType GOP_TIMECODE value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::spherical) == AV_FRAME_DATA_SPHERICAL, "FrameSideDataType SPHERICAL value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::contentLightLevel) == AV_FRAME_DATA_CONTENT_LIGHT_LEVEL, "FrameSideDataType CONTENT_LIGHT_LEVEL value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::iccProfile) == AV_FRAME_DATA_ICC_PROFILE, "FrameSideDataType ICC_PROFILE value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::qpTablePrototypes) == AV_FRAME_DATA_QP_TABLE_PROPERTIES, "FrameSideDataType QP_TABLE_PROPERTIES value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::qpTableData) == AV_FRAME_DATA_QP_TABLE_DATA, "FrameSideDataType QP_TABLE_DATA value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::S12MTimecode) == AV_FRAME_DATA_S12M_TIMECODE, "FrameSideDataType S12M_TIMECODE value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::dynamicHdrPlus) == AV_FRAME_DATA_DYNAMIC_HDR_PLUS, "FrameSideDataType DYNAMIC_HDR_PLUS value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::regionsOfInterest) == AV_FRAME_DATA_REGIONS_OF_INTEREST, "FrameSideDataType REGIONS_OF_INTEREST value must match");
static_assert(static_cast<AVFrameSideDataType>(FrameSideDataType::videoEncParams) == AV_FRAME_DATA_VIDEO_ENC_PARAMS, "FrameSideDataType VIDEO_ENC_PARAMS value must match");

static_assert(sizeof(AVDiscard) == sizeof(Discard), "Discard enum's size does not match");
static_assert(static_cast<AVDiscard>(Discard::none) == AVDISCARD_NONE, "Discard null value must match");

static_assert(static_cast<int>(ThreadType::none) == 0, "ThreadType null value must match");
static_assert(static_cast<int>(ThreadType::frame) == FF_THREAD_FRAME, "ThreadType FRAME value must match");
static_assert(static_cast<int>(ThreadType::slice) == FF_THREAD_SLICE, "ThreadType SLICE value must match");

static_assert(static_cast<int>(SeekFlags::none) == 0, "Seek null value must match");
static_assert(static_cast<int>(SeekFlags::backward) == AVSEEK_FLAG_BACKWARD, "Seek BACKWARD value must match");
static_assert(static_cast<int>(SeekFlags::byte) == AVSEEK_FLAG_BYTE, "Seek BYTE value must match");
static_assert(static_cast<int>(SeekFlags::any) == AVSEEK_FLAG_ANY, "Seek ANY value must match");
static_assert(static_cast<int>(SeekFlags::frame) == AVSEEK_FLAG_FRAME, "Seek FRAME value must match");

static_assert(static_cast<int>(HWDeviceType::none) == AV_HWDEVICE_TYPE_NONE, "Hardware device type none value must match");
static_assert(static_cast<int>(HWDeviceType::vdpau) == AV_HWDEVICE_TYPE_VDPAU, "Hardware device type VDPAU value must match");
static_assert(static_cast<int>(HWDeviceType::cuda) == AV_HWDEVICE_TYPE_CUDA, "Hardware device type CUDA value must match");
static_assert(static_cast<int>(HWDeviceType::vaapi) == AV_HWDEVICE_TYPE_VAAPI, "Hardware device type VAAPI value must match");
static_assert(static_cast<int>(HWDeviceType::dxva2) == AV_HWDEVICE_TYPE_DXVA2, "Hardware device type DXVA2 value must match");
static_assert(static_cast<int>(HWDeviceType::qsv) == AV_HWDEVICE_TYPE_QSV, "Hardware device type QSV value must match");
static_assert(static_cast<int>(HWDeviceType::videotoolbox) == AV_HWDEVICE_TYPE_VIDEOTOOLBOX, "Hardware device type VIDEOTOOLBOX value must match");
static_assert(static_cast<int>(HWDeviceType::d3D11va) == AV_HWDEVICE_TYPE_D3D11VA, "Hardware device type D3D11VA value must match");
static_assert(static_cast<int>(HWDeviceType::drm) == AV_HWDEVICE_TYPE_DRM, "Hardware device type DRM value must match");
static_assert(static_cast<int>(HWDeviceType::opencl) == AV_HWDEVICE_TYPE_OPENCL, "Hardware device type OPENCL value must match");
static_assert(static_cast<int>(HWDeviceType::mediacodec) == AV_HWDEVICE_TYPE_MEDIACODEC, "Hardware device type MEDIACODEC value must match");
static_assert(static_cast<int>(HWDeviceType::vulkan) == AV_HWDEVICE_TYPE_VULKAN, "Hardware device type VULKAN value must match");


MediaType getMediaType(CodecID id) {
	return static_cast<MediaType>(avcodec_get_type(static_cast<AVCodecID>(id)));
}

std::string_view getProfileName(CodecID id, int profile) {
	return avcodec_profile_name(static_cast<AVCodecID>(id), profile);
}



std::ostream& operator<<(std::ostream& os, MediaType type) {
	return os << toString(type);
}

std::ostream& operator<<(std::ostream& os, CodecID id) {
	return os << toString(id);
}

std::ostream& operator<<(std::ostream& os, PixelFormat fmt) {
	return os << toString(fmt);
}

std::ostream& operator<<(std::ostream& os, ColorPrimaries prim) {
	return os << toString(prim);
}

std::ostream& operator<<(std::ostream& os, ColorTransferCharacteristic trc) {
	return os << toString(trc);
}

std::ostream& operator<<(std::ostream& os, ColorSpace space) {
	return os << toString(space);
}

std::ostream& operator<<(std::ostream& os, ColorRange range) {
	return os << toString(range);
}

std::ostream& operator<<(std::ostream& os, ChromaLocation loc) {
	return os << toString(loc);
}

std::ostream& operator<<(std::ostream& os, PictureType type) {
	char str = toString(type);
	return os << std::string_view(&str, 1);
}

std::ostream& operator<<(std::ostream& os, HWDeviceType type) {
	return os << toString(type);
}

}

namespace Zuazo {

std::string_view toString(FFmpeg::MediaType type) {
	return std::string_view(av_get_media_type_string(static_cast<AVMediaType>(type)));
}

std::string_view toString(FFmpeg::CodecID id) {
	return std::string_view(avcodec_get_name(static_cast<AVCodecID>(id)));
}

std::string_view toString(FFmpeg::PixelFormat fmt) {
	return std::string_view(av_get_pix_fmt_name(static_cast<AVPixelFormat>(fmt)));
}

std::string_view toString(FFmpeg::ColorPrimaries prim) {
	return std::string_view(av_color_primaries_name(static_cast<AVColorPrimaries>(prim)));
}

std::string_view toString(FFmpeg::ColorTransferCharacteristic trc) {
	return std::string_view(av_color_transfer_name(static_cast<AVColorTransferCharacteristic>(trc)));
}

std::string_view toString(FFmpeg::ColorSpace space) {
	return std::string_view(av_color_space_name(static_cast<AVColorSpace>(space)));
}

std::string_view toString(FFmpeg::ColorRange range) {
	return std::string_view(av_color_range_name(static_cast<AVColorRange>(range)));
}

std::string_view toString(FFmpeg::ChromaLocation loc) {
	return std::string_view(av_chroma_location_name(static_cast<AVChromaLocation>(loc)));
}

char toString(FFmpeg::PictureType type) {
	return av_get_picture_type_char(static_cast<AVPictureType>(type));
}

std::string_view toString(FFmpeg::HWDeviceType type) {
	return std::string_view(av_hwdevice_get_type_name(static_cast<AVHWDeviceType>(type)));
}

}
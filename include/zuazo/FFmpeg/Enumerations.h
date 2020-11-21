#pragma once

#include <zuazo/Macros.h>
#include <zuazo/Utils/Bit.h>

#include <string_view>

namespace Zuazo::FFmpeg {

enum class MediaType {
	NONE = -1,
	VIDEO = 0,
	AUDIO = 1,
	DATA = 2,
	SUBTITLE = 3,
	ATTACHMENT = 4,
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(MediaType)
ZUAZO_ENUM_COMP_OPERATORS(MediaType)	

std::ostream& operator<<(std::ostream& os, MediaType type);



enum class CodecID {
	NONE = 0
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(CodecID)
ZUAZO_ENUM_COMP_OPERATORS(CodecID)	

MediaType getMediaType(CodecID id);
std::string_view getProfileName(CodecID id, int profile);

std::ostream& operator<<(std::ostream& os, CodecID id);



enum class PixelFormat {
	NONE = -1
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(PixelFormat)
ZUAZO_ENUM_COMP_OPERATORS(PixelFormat)	

std::ostream& operator<<(std::ostream& os, PixelFormat fmt);



enum class ColorPrimaries {
	NONE = 2
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ColorPrimaries)
ZUAZO_ENUM_COMP_OPERATORS(ColorPrimaries)	

std::ostream& operator<<(std::ostream& os, ColorPrimaries prim);



enum class ColorTransferCharacteristic {
	NONE = 2
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ColorTransferCharacteristic)
ZUAZO_ENUM_COMP_OPERATORS(ColorTransferCharacteristic)	

std::ostream& operator<<(std::ostream& os, ColorTransferCharacteristic trc);



enum class ColorSpace {
	NONE = 2
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ColorSpace)
ZUAZO_ENUM_COMP_OPERATORS(ColorSpace)	

std::ostream& operator<<(std::ostream& os, ColorSpace space);



enum class ColorRange {
	NONE = 0
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ColorRange)
ZUAZO_ENUM_COMP_OPERATORS(ColorRange)	

std::ostream& operator<<(std::ostream& os, ColorRange range);



enum class ChromaLocation {
	NONE = 0
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ChromaLocation)
ZUAZO_ENUM_COMP_OPERATORS(ChromaLocation)	

std::ostream& operator<<(std::ostream& os, ChromaLocation loc);



enum class FieldOrder {
	NONE = 0,
	PROGRESSIVE = 1
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(FieldOrder)
ZUAZO_ENUM_COMP_OPERATORS(FieldOrder)	

//std::ostream& operator<<(std::ostream& os, FieldOrder order);



enum class PictureType {
	NONE = 0,
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(PictureType)
ZUAZO_ENUM_COMP_OPERATORS(PictureType)	

std::ostream& operator<<(std::ostream& os, PictureType type);



enum class FrameSideDataType {
	PANSCAN,
	A53_CC,
	STEREO3D,
	MATRIXENCODING,
	DOWNMIX_INFO,
	REPLAYGAIN,
	DISPLAYMATRIX,
	AFD,
	MOTION_VECTORS,
	SKIP_SAMPLES,
	AUDIO_SERVICE_TYPE,
	MASTERING_DISPLAY_METADATA,
	GOP_TIMECODE,
	SPHERICAL,
	CONTENT_LIGHT_LEVEL,
	ICC_PROFILE,
	QP_TABLE_PROPERTIES,
	QP_TABLE_DATA,
	S12M_TIMECODE,
	DYNAMIC_HDR_PLUS,
	REGIONS_OF_INTEREST,
	VIDEO_ENC_PARAMS,

};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(FrameSideDataType)
ZUAZO_ENUM_COMP_OPERATORS(FrameSideDataType)	

//std::ostream& operator<<(std::ostream& os, FrameSideData type);



enum class Discard {
	NONE = -16,
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(Discard)
ZUAZO_ENUM_COMP_OPERATORS(Discard)	

//std::ostream& operator<<(std::ostream& os, Discard disc);



enum class ThreadType {
	NONE = 0,
	FRAME = 1,
	SLICE = 2
};

ZUAZO_ENUM_BIT_OPERATORS(ThreadType)

//std::ostream& operator<<(std::ostream& os, ThreadType disc);



enum class SeekFlags {
	NONE		= 0,
	BACKWARD 	= Utils::bit(0),
	BYTE		= Utils::bit(1),
	ANY			= Utils::bit(2),
	FRAME		= Utils::bit(3),
};

ZUAZO_ENUM_BIT_OPERATORS(SeekFlags)

//std::ostream& operator<<(std::ostream& os, SeekFlags seek);

}

namespace Zuazo {

std::string_view toString(FFmpeg::MediaType type);
std::string_view toString(FFmpeg::CodecID id);
std::string_view toString(FFmpeg::PixelFormat fmt);
std::string_view toString(FFmpeg::ColorPrimaries prim);
std::string_view toString(FFmpeg::ColorTransferCharacteristic trc);
std::string_view toString(FFmpeg::ColorSpace space);
std::string_view toString(FFmpeg::ColorRange range);
std::string_view toString(FFmpeg::ChromaLocation loc);
//std::string_view toString(FFmpeg::FieldOrder order);
char toString(FFmpeg::PictureType type);
//std::string_view toString(FFmpeg::FrameSideDataType type);
//std::string_view toString(FFmpeg::Discard disc);
//std::string_view toString(FFmpeg::ThreadType disc);
//std::string_view toString(FFmpeg::SeekFlags seek);


}
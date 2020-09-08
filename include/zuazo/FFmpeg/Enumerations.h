#pragma once

#include <zuazo/Macros.h>

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



enum class CodecID {
	NONE = 0
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(CodecID)
ZUAZO_ENUM_COMP_OPERATORS(CodecID)	

MediaType getMediaType(FFmpeg::CodecID id);
std::string_view getProfileName(FFmpeg::CodecID id, int profile);



enum class PixelFormat {
	NONE = -1
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(PixelFormat)
ZUAZO_ENUM_COMP_OPERATORS(PixelFormat)	



enum class ColorPrimaries {
	NONE = 2
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ColorPrimaries)
ZUAZO_ENUM_COMP_OPERATORS(ColorPrimaries)	



enum class ColorTransferCharacteristic {
	NONE = 2
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ColorTransferCharacteristic)
ZUAZO_ENUM_COMP_OPERATORS(ColorTransferCharacteristic)	



enum class ColorSpace {
	NONE = 2
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ColorSpace)
ZUAZO_ENUM_COMP_OPERATORS(ColorSpace)	



enum class ColorRange {
	NONE = 0
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ColorRange)
ZUAZO_ENUM_COMP_OPERATORS(ColorRange)	



enum class ChromaLocation {
	NONE = 0
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ChromaLocation)
ZUAZO_ENUM_COMP_OPERATORS(ChromaLocation)	



enum class FieldOrder {
	NONE = 0,
	PROGRESSIVE = 1
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(FieldOrder)
ZUAZO_ENUM_COMP_OPERATORS(FieldOrder)	



enum class PictureType {
	NONE = 0,
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(PictureType)
ZUAZO_ENUM_COMP_OPERATORS(PictureType)	



enum class Discard {
	NONE = -16,
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(Discard)
ZUAZO_ENUM_COMP_OPERATORS(Discard)	



enum class ThreadType {
	NONE = 0,
	FRAME = 1,
	SLICE = 2
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ThreadType)
ZUAZO_ENUM_COMP_OPERATORS(ThreadType)	

}

namespace Zuazo {

std::string_view toString(FFmpeg::MediaType type);
std::ostream& operator<<(std::ostream& os, FFmpeg::MediaType type);

std::string_view toString(FFmpeg::CodecID id);
std::ostream& operator<<(std::ostream& os, FFmpeg::CodecID id);

std::string_view toString(FFmpeg::PixelFormat fmt);
std::ostream& operator<<(std::ostream& os, FFmpeg::PixelFormat fmt);

std::string_view toString(FFmpeg::ColorPrimaries prim);
std::ostream& operator<<(std::ostream& os, FFmpeg::ColorPrimaries prim);

std::string_view toString(FFmpeg::ColorTransferCharacteristic trc);
std::ostream& operator<<(std::ostream& os, FFmpeg::ColorTransferCharacteristic trc);

std::string_view toString(FFmpeg::ColorSpace space);
std::ostream& operator<<(std::ostream& os, FFmpeg::ColorSpace space);

std::string_view toString(FFmpeg::ColorRange range);
std::ostream& operator<<(std::ostream& os, FFmpeg::ColorRange range);

std::string_view toString(FFmpeg::ChromaLocation loc);
std::ostream& operator<<(std::ostream& os, FFmpeg::ChromaLocation loc);

//std::string_view toString(FFmpeg::FieldOrder order);
//std::ostream& operator<<(std::ostream& os, FFmpeg::FieldOrder order);

char toString(FFmpeg::PictureType type);
std::ostream& operator<<(std::ostream& os, FFmpeg::PictureType type);

//std::string_view toString(FFmpeg::Discard disc);
//std::ostream& operator<<(std::ostream& os, FFmpeg::Discard disc);

//std::string_view toString(FFmpeg::ThreadType disc);
//std::ostream& operator<<(std::ostream& os, FFmpeg::ThreadType disc);

}
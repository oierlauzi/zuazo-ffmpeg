#pragma once

#include <zuazo/Macros.h>
#include <zuazo/Utils/Bit.h>

#include <string_view>

namespace Zuazo::FFmpeg {

enum class MediaType : int {
	none = -1,
	video = 0,
	audio = 1,
	data = 2,
	subtitle = 3,
	attachment = 4,
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(MediaType)
ZUAZO_ENUM_COMP_OPERATORS(MediaType)	

std::ostream& operator<<(std::ostream& os, MediaType type);



enum class CodecID : int {
	none = 0
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(CodecID)
ZUAZO_ENUM_COMP_OPERATORS(CodecID)	

MediaType getMediaType(CodecID id);
std::string_view getProfileName(CodecID id, int profile);

std::ostream& operator<<(std::ostream& os, CodecID id);



enum class PixelFormat : int {
	none = -1
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(PixelFormat)
ZUAZO_ENUM_COMP_OPERATORS(PixelFormat)	

std::ostream& operator<<(std::ostream& os, PixelFormat fmt);



enum class ColorPrimaries : int {
	none = 2
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ColorPrimaries)
ZUAZO_ENUM_COMP_OPERATORS(ColorPrimaries)	

std::ostream& operator<<(std::ostream& os, ColorPrimaries prim);



enum class ColorTransferCharacteristic : int {
	none = 2
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ColorTransferCharacteristic)
ZUAZO_ENUM_COMP_OPERATORS(ColorTransferCharacteristic)	

std::ostream& operator<<(std::ostream& os, ColorTransferCharacteristic trc);



enum class ColorSpace : int {
	none = 2
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ColorSpace)
ZUAZO_ENUM_COMP_OPERATORS(ColorSpace)	

std::ostream& operator<<(std::ostream& os, ColorSpace space);



enum class ColorRange : int {
	none = 0
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ColorRange)
ZUAZO_ENUM_COMP_OPERATORS(ColorRange)	

std::ostream& operator<<(std::ostream& os, ColorRange range);



enum class ChromaLocation : int {
	none = 0
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(ChromaLocation)
ZUAZO_ENUM_COMP_OPERATORS(ChromaLocation)	

std::ostream& operator<<(std::ostream& os, ChromaLocation loc);



enum class FieldOrder : int {
	none = 0,
	progressive = 1
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(FieldOrder)
ZUAZO_ENUM_COMP_OPERATORS(FieldOrder)	

//std::ostream& operator<<(std::ostream& os, FieldOrder order);



enum class PictureType : int {
	none = 0,
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(PictureType)
ZUAZO_ENUM_COMP_OPERATORS(PictureType)	

std::ostream& operator<<(std::ostream& os, PictureType type);



enum class FrameSideDataType : int {
	panscan,
	A53cc,
	stereo3D,
	matrixEncoding,
	downmixInfo,
	replayGain,
	displayMatrix,
	sfd,
	motionVectors,
	skipSamples,
	audioServiceType,
	masteringDisplayMetadata,
	gopTimecode,
	spherical,
	contentLightLevel,
	iccProfile,
	qpTablePrototypes,
	qpTableData,
	S12MTimecode,
	dynamicHdrPlus,
	regionsOfInterest,
	videoEncParams,

};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(FrameSideDataType)
ZUAZO_ENUM_COMP_OPERATORS(FrameSideDataType)	

//std::ostream& operator<<(std::ostream& os, FrameSideData type);



enum class Discard : int {
	none = -16,
};

ZUAZO_ENUM_ARITHMETIC_OPERATORS(Discard)
ZUAZO_ENUM_COMP_OPERATORS(Discard)	

//std::ostream& operator<<(std::ostream& os, Discard disc);



enum class ThreadType : int {
	none = 0,
	frame = 1,
	slice = 2
};

ZUAZO_ENUM_BIT_OPERATORS(ThreadType)

//std::ostream& operator<<(std::ostream& os, ThreadType disc);



enum class SeekFlags : int {
	none		= 0,
	backward 	= Utils::bit(0),
	byte		= Utils::bit(1),
	any			= Utils::bit(2),
	frame		= Utils::bit(3),
};

ZUAZO_ENUM_BIT_OPERATORS(SeekFlags)

//std::ostream& operator<<(std::ostream& os, SeekFlags seek);



enum class HWDeviceType : int {
	none			= 0, 
	vdpau			= 1,
	cuda			= 2,
	vaapi			= 3,
	dxva2			= 4,
	qsv				= 5,
	videotoolbox	= 6,
	d3D11va			= 7,
	drm				= 8,
	opencl			= 9,
	mediacodec		= 10,
	vulkan			= 11
};

ZUAZO_ENUM_BIT_OPERATORS(HWDeviceType)

std::ostream& operator<<(std::ostream& os, HWDeviceType type);

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
std::string_view toString(FFmpeg::HWDeviceType type);

}
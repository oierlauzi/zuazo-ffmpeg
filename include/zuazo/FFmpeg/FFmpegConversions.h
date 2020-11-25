#pragma once

#include "Enumerations.h"

#include <zuazo/Chrono.h>
#include <zuazo/ColorPrimaries.h>
#include <zuazo/ColorModel.h>
#include <zuazo/ColorTransferFunction.h>
#include <zuazo/ColorRange.h>
#include <zuazo/ColorSubsampling.h>
#include <zuazo/ColorFormat.h>

#include <tuple>

namespace Zuazo::FFmpeg {

ColorPrimaries toFFmpeg(Zuazo::ColorPrimaries prim);
Zuazo::ColorPrimaries fromFFmpeg(ColorPrimaries prim);

ColorSpace toFFmpeg(Zuazo::ColorModel model);
Zuazo::ColorModel fromFFmpeg(ColorSpace model);

ColorTransferCharacteristic toFFmpeg(Zuazo::ColorTransferFunction func);
Zuazo::ColorTransferFunction fromFFmpeg(ColorTransferCharacteristic func);

ColorRange toFFmpeg(Zuazo::ColorRange range);
Zuazo::ColorRange fromFFmpeg(ColorRange range);

struct PixelFormatConversion {
	ColorFormat colorFormat;
	ColorSubsampling colorSubsampling;
	bool isYCbCr;
};

PixelFormat toFFmpeg(const PixelFormatConversion& fmt);
PixelFormatConversion fromFFmpeg(PixelFormat fmt);

HWDeviceType getHardwareDeviceType(PixelFormat fmt);

ColorSubsampling subsamplingFromLog2(uint8_t hor, uint8_t vert);
bool isHardwarePixelFormat(FFmpeg::PixelFormat fmt);
bool isRGBPixelFormat(FFmpeg::PixelFormat fmt);

}

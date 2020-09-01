#pragma once

#include <zuazo/Math/Rational.h>
#include <zuazo/ColorFormat.h>
#include <zuazo/ColorSubsampling.h>
#include <zuazo/ColorModel.h>
#include <zuazo/FFmpeg/MediaType.h>
#include <zuazo/FFmpeg/CodecID.h>

#include <tuple>

extern "C" {
	#include <libavutil/rational.h>
	#include <libavutil/pixfmt.h>
	#include <libavutil/avutil.h>

	#include <libavcodec/codec_id.h>

}

namespace Zuazo::FFmpeg {

using Rational = Math::Rational<decltype(AVRational::num), decltype(AVRational::den)>;
constexpr AVRational toFFmpeg(Rational rat);
constexpr Rational fromFFmpeg(AVRational rat);

constexpr AVPixelFormat toFFmpeg(ColorFormat fmt, ColorSubsampling subs, ColorModel model);
constexpr std::tuple<ColorFormat, ColorSubsampling> fromFFmpeg(AVPixelFormat fmt);

constexpr AVMediaType toFFmpeg(MediaType type);
constexpr MediaType fromFFmpeg(AVMediaType type);

constexpr AVCodecID toFFmpeg(CodecId id);
constexpr CodecId fromFFmpeg(AVCodecID id);

}

#include "FFmpegConversions.inl"
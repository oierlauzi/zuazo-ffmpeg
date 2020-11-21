#include <zuazo/FFmpeg/FFmpegConversions.h>

#include <zuazo/Utils/CPU.h>

extern "C" {
	#include <libavutil/avutil.h>
	#include <libavutil/pixfmt.h>
	#include <libavutil/pixdesc.h>
	#include <libavcodec/codec_id.h>
	#include <libavcodec/avcodec.h>
}

namespace Zuazo::FFmpeg {


static AVColorPrimaries toFFmpegLUT(Zuazo::ColorPrimaries prim) {
	switch(prim) {
	case Zuazo::ColorPrimaries::BT601_625:		return AVCOL_PRI_BT470BG;
	case Zuazo::ColorPrimaries::BT601_525:		return AVCOL_PRI_SMPTE170M;
	case Zuazo::ColorPrimaries::BT709:			return AVCOL_PRI_BT709;
	case Zuazo::ColorPrimaries::BT2020:			return AVCOL_PRI_BT2020;
	case Zuazo::ColorPrimaries::SMPTE431:		return AVCOL_PRI_SMPTE431;
	case Zuazo::ColorPrimaries::SMPTE432:		return AVCOL_PRI_SMPTE432;
	//case ColorPrimaries::ADOBE_RGB:			return {}; /*NOT SUPPORTED*/

	default: 									return AVCOL_PRI_UNSPECIFIED;
	}
}

ColorPrimaries toFFmpeg(Zuazo::ColorPrimaries prim) {
	return static_cast<ColorPrimaries>(toFFmpegLUT(prim));
}


static Zuazo::ColorPrimaries fromFFmpegLUT(AVColorPrimaries prim) {
	/*
	 * Comments have been obtained from:
	 * libavutil/pixfmt.h
	 */

	switch(prim) {
	case AVCOL_PRI_BT709:       return Zuazo::ColorPrimaries::BT709;		//also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
	//case AVCOL_PRI_BT470M:    return {}; /*NOT SUPPORTED*/ 				//also FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
	case AVCOL_PRI_BT470BG:   	return Zuazo::ColorPrimaries::BT601_625;	//also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
	case AVCOL_PRI_SMPTE170M: 	return Zuazo::ColorPrimaries::BT601_525; 	//also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
	case AVCOL_PRI_SMPTE240M: 	return Zuazo::ColorPrimaries::BT601_525; 	//functionally identical to above
	//case AVCOL_PRI_FILM:      return {}; /*NOT SUPPORTED*/ 				//colour filters using Illuminant C
	case AVCOL_PRI_BT2020:      return Zuazo::ColorPrimaries::BT2020;		//ITU-R BT2020
	//case AVCOL_PRI_SMPTE428:  return {}; /*NOT SUPPORTED*/ 				//SMPTE ST 428-1 (CIE 1931 XYZ)
	case AVCOL_PRI_SMPTE431:    return Zuazo::ColorPrimaries::SMPTE431;		//SMPTE ST 431-2 (2011) / DCI P3
	case AVCOL_PRI_SMPTE432:    return Zuazo::ColorPrimaries::SMPTE432;		//SMPTE ST 432-1 (2010) / P3 D65 / Display P3
	//case AVCOL_PRI_EBU3213:   return {}; /*NOT SUPPORTED*/ 				//EBU Tech. 3213-E / JEDEC P22 phosphors

	default: 					return Zuazo::ColorPrimaries::NONE;
	}
}

Zuazo::ColorPrimaries fromFFmpeg(ColorPrimaries prim) {
	return fromFFmpegLUT(static_cast<AVColorPrimaries>(prim));
}



static AVColorSpace toFFmpegLUT(Zuazo::ColorModel model) {
	switch(model) {
	case Zuazo::ColorModel::RGB:		return AVCOL_SPC_RGB;
	case Zuazo::ColorModel::BT601:		return AVCOL_SPC_BT470BG;
	case Zuazo::ColorModel::BT709:		return AVCOL_SPC_BT709;
	case Zuazo::ColorModel::BT2020:		return AVCOL_SPC_BT2020_NCL;
	case Zuazo::ColorModel::SMPTE240M:	return AVCOL_SPC_SMPTE240M;

	default: 							return AVCOL_SPC_UNSPECIFIED;
	}
}

ColorSpace toFFmpeg(Zuazo::ColorModel model) {
	return static_cast<ColorSpace>(toFFmpegLUT(model));
}


static Zuazo::ColorModel fromFFmpegLUT(AVColorSpace model) {
	/*
	 * Comments have been obtained from:
	 * libavutil/pixfmt.h
	 */

	switch(model) {
	case AVCOL_SPC_RGB:					return Zuazo::ColorModel::RGB;			//order of coefficients is actually GBR, also IEC 61966-2-1 (sRGB)
	case AVCOL_SPC_BT709:				return Zuazo::ColorModel::BT709;		//also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
	//case AVCOL_SPC_FCC:				return {}; /*NOT SUPPORTED*/			//FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
	case AVCOL_SPC_BT470BG:				return Zuazo::ColorModel::BT601;		//also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601
	case AVCOL_SPC_SMPTE170M:			return Zuazo::ColorModel::SMPTE240M;	//also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
	case AVCOL_SPC_SMPTE240M:			return Zuazo::ColorModel::SMPTE240M;	//functionally identical to above
	//case AVCOL_SPC_YCGCO:				return {}; /*NOT SUPPORTED*/			//Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16					
	case AVCOL_SPC_BT2020_NCL:			return Zuazo::ColorModel::BT2020;		//ITU-R BT2020 non-constant luminance system
	//case AVCOL_SPC_BT2020_CL:			return {}; /*NOT SUPPORTED*/			//ITU-R BT2020 constant luminance system
	//case AVCOL_SPC_SMPTE2085:			return {}; /*NOT SUPPORTED*/			//SMPTE 2085, Y'D'zD'x
	//case AVCOL_SPC_CHROMA_DERIVED_NCL:return {}; /*NOT SUPPORTED*/			//Chromaticity-derived non-constant luminance system
	//case AVCOL_SPC_CHROMA_DERIVED_CL:	return {}; /*NOT SUPPORTED*/			//Chromaticity-derived constant luminance system
	//case AVCOL_SPC_ICTCP:				return {}; /*NOT SUPPORTED*/			//ITU-R BT.2100-0, ICtCp

	default: 							return Zuazo::ColorModel::NONE;
	}
}

Zuazo::ColorModel fromFFmpeg(ColorSpace model) {
	return fromFFmpegLUT(static_cast<AVColorSpace>(model));
}



static AVColorTransferCharacteristic toFFmpegLUT(Zuazo::ColorTransferFunction func) {
	switch(func) {
	case Zuazo::ColorTransferFunction::LINEAR:			return AVCOL_TRC_LINEAR;
	case Zuazo::ColorTransferFunction::BT601:			return AVCOL_TRC_SMPTE170M;
	case Zuazo::ColorTransferFunction::BT709:			return AVCOL_TRC_BT709;
	case Zuazo::ColorTransferFunction::BT2020_10:		return AVCOL_TRC_BT2020_10;
	case Zuazo::ColorTransferFunction::BT2020_12:		return AVCOL_TRC_BT2020_12;
	case Zuazo::ColorTransferFunction::GAMMA22:			return AVCOL_TRC_GAMMA22;
	//case Zuazo::ColorTransferFunction::GAMMA26:		return {}; /*NOT SUPPORTED*/
	case Zuazo::ColorTransferFunction::GAMMA28:			return AVCOL_TRC_GAMMA28;
	case Zuazo::ColorTransferFunction::IEC61966_2_1:	return AVCOL_TRC_IEC61966_2_1;
	case Zuazo::ColorTransferFunction::IEC61966_2_4:	return AVCOL_TRC_IEC61966_2_4;
	case Zuazo::ColorTransferFunction::SMPTE240M:		return AVCOL_TRC_SMPTE240M;
	case Zuazo::ColorTransferFunction::SMPTE2084:		return AVCOL_TRC_SMPTE2084;
	case Zuazo::ColorTransferFunction::ARIB_STD_B67:	return AVCOL_TRC_ARIB_STD_B67;

	default: 											return AVCOL_TRC_UNSPECIFIED;
	}
}

ColorTransferCharacteristic toFFmpeg(Zuazo::ColorTransferFunction func) {
	return static_cast<ColorTransferCharacteristic>(toFFmpegLUT(func));
}


static Zuazo::ColorTransferFunction fromFFmpegLUT(AVColorTransferCharacteristic func) {
	/*
	 * Comments have been obtained from:
	 * libavutil/pixfmt.h
	 */

	switch(func) {
    case AVCOL_TRC_BT709:			return Zuazo::ColorTransferFunction::BT709;			//also ITU-R BT1361
    case AVCOL_TRC_GAMMA22:			return Zuazo::ColorTransferFunction::GAMMA22;		//also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
    case AVCOL_TRC_GAMMA28:			return Zuazo::ColorTransferFunction::GAMMA28; 		//also ITU-R BT470BG
    case AVCOL_TRC_SMPTE170M:		return Zuazo::ColorTransferFunction::BT601;			//also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
    case AVCOL_TRC_SMPTE240M:		return Zuazo::ColorTransferFunction::SMPTE240M;	
    case AVCOL_TRC_LINEAR:			return Zuazo::ColorTransferFunction::LINEAR;		//"Linear transfer characteristics"
    //case AVCOL_TRC_LOG:			return {}; /*NOT SUPPORTED*/ 						//"Logarithmic transfer characteristic (100:1 range)"
    //case AVCOL_TRC_LOG_SQRT:		return {}; /*NOT SUPPORTED*/ 						//"Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)"
    case AVCOL_TRC_IEC61966_2_4:	return Zuazo::ColorTransferFunction::IEC61966_2_4;	//IEC 61966-2-4
    //case AVCOL_TRC_BT1361_ECG:	return {}; /*NOT SUPPORTED*/ 						//ITU-R BT1361 Extended Colour Gamut
    case AVCOL_TRC_IEC61966_2_1:	return Zuazo::ColorTransferFunction::IEC61966_2_1;	//IEC 61966-2-1 (sRGB or sYCC)
    case AVCOL_TRC_BT2020_10:		return Zuazo::ColorTransferFunction::BT2020_10; 	//ITU-R BT2020 for 10-bit system
    case AVCOL_TRC_BT2020_12:		return Zuazo::ColorTransferFunction::BT2020_12; 	//ITU-R BT2020 for 12-bit system
    case AVCOL_TRC_SMPTE2084:		return Zuazo::ColorTransferFunction::SMPTE2084; 	//SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems
    //case AVCOL_TRC_SMPTE428:		return {}; /*NOT SUPPORTED*/ 						//SMPTE ST 428-1
    case AVCOL_TRC_ARIB_STD_B67:	return Zuazo::ColorTransferFunction::ARIB_STD_B67; 	//ARIB STD-B67, known as "Hybrid log-gamma"

	default: 						return Zuazo::ColorTransferFunction::NONE;
	}
}

Zuazo::ColorTransferFunction fromFFmpeg(ColorTransferCharacteristic func) {
	return fromFFmpegLUT(static_cast<AVColorTransferCharacteristic>(func));
}



static AVColorRange toFFmpegLUT(Zuazo::ColorRange range) {
	switch(range) {
	case Zuazo::ColorRange::FULL:		return AVCOL_RANGE_JPEG;
	case Zuazo::ColorRange::ITU_NARROW:	return AVCOL_RANGE_MPEG;
	default: 							return AVCOL_RANGE_UNSPECIFIED;
	}
}

ColorRange toFFmpeg(Zuazo::ColorRange range) {
	return static_cast<ColorRange>(toFFmpegLUT(range));
}


static Zuazo::ColorRange fromFFmpegLUT(AVColorRange range) {
	switch(range) {
	case AVCOL_RANGE_JPEG:	return Zuazo::ColorRange::FULL;
	case AVCOL_RANGE_MPEG: 	return Zuazo::ColorRange::ITU_NARROW;
	default: 				return Zuazo::ColorRange::NONE;
	}
}

Zuazo::ColorRange fromFFmpeg(ColorRange range) {
	return fromFFmpegLUT(static_cast<AVColorRange>(range));
}



static AVPixelFormat toFFmpegLUT(const PixelFormatConversion& fmt) {
	if(fmt.isYCbCr) {
		switch(fmt.colorFormat) {
		//8bpc
		case ColorFormat::B8G8R8G8:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_UYVY422;
			default: break;
			}
			break;

		case ColorFormat::G8R8G8B8:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YVYU422;
			default: break;
			}
			break;

		case ColorFormat::G8B8G8R8:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YUYV422;
			default: break;
			}
			break;

		case ColorFormat::G8_B8_R8_A8:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_YUVA444P;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YUVA422P;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_YUVA420P;
			default: break;
			}
			break;

		case ColorFormat::G8_B8_R8:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_YUV444P;
			case ColorSubsampling::RB_440:			return AV_PIX_FMT_YUV440P;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YUV422P;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_YUV420P;
			case ColorSubsampling::RB_411:			return AV_PIX_FMT_YUV411P;
			case ColorSubsampling::RB_410:			return AV_PIX_FMT_YUV410P;
			default: break;
			}
			break;

		case ColorFormat::G8_B8R8:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_NV24;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_NV16;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_NV12;
			default: break;
			}
			break;

		case ColorFormat::G8_R8B8:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_NV42;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_NV21;
			default: break;
			}
			break;


		//10bpc
		case ColorFormat::G10X6B10X6G10X6R10X6_16:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_Y210;
			default: break;
			}
			break;
		
		case ColorFormat::G10X6_B10X6_R10X6_A10X6_16:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_YUVA444P10;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YUVA422P10;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_YUVA420P10;
			default: break;
			}
			break;

		case ColorFormat::G10X6_B10X6_R10X6_16:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_YUV444P10;
			case ColorSubsampling::RB_440:			return AV_PIX_FMT_YUV440P10;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YUV422P10;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_YUV420P10;
			default: break;
			}
			break;

		case ColorFormat::G10X6_B10X6R10X6_16:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_NV20;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_P010;
			default: break;
			}
			break;


		//12bpc	
		case ColorFormat::G12X4_B12X4_R12X4_A12X4_16:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_YUVA444P12;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YUVA422P12;
			default: break;
			}
			break;

		case ColorFormat::G12X4_B12X4_R12X4_16:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_YUV444P12;
			case ColorSubsampling::RB_440:			return AV_PIX_FMT_YUV440P12;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YUV422P12;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_YUV420P12;
			default: break;
			}
			break;


		//16bpc
		case ColorFormat::A16G16B16R16:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_AYUV64;
			default: break;
			}
			break;

		case ColorFormat::G16_B16_R16_A16:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_YUVA444P16;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YUVA422P16;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_YUVA420P16;
			default: break;
			}
			break;

		case ColorFormat::G16_B16_R16:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_YUV444P16;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YUV422P16;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_YUV420P16;
			default: break;
			}
			break;

		case ColorFormat::G16_B16R16:
			switch (fmt.colorSubsampling) {
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_P016;
			default: break;
			}
			break;


		default: break;
		}
	} else if(fmt.colorSubsampling == ColorSubsampling::RB_444) {
		//RGB with no subsampling
		switch(fmt.colorFormat) {
		//4bpc
		case ColorFormat::X4R4G4B4_16: 				return AV_PIX_FMT_RGB444;
		case ColorFormat::X4B4G4R4_16: 				return AV_PIX_FMT_BGR444;

		//5bpc
		case ColorFormat::X1R5G5B5_16: 				return AV_PIX_FMT_RGB555;
		case ColorFormat::X1B5G5R5_16: 				return AV_PIX_FMT_BGR555;

		//6bpc
		case ColorFormat::R5G6B5_16: 				return AV_PIX_FMT_RGB565;
		case ColorFormat::B5G6R5_16: 				return AV_PIX_FMT_BGR565;

		//8bpc
		case ColorFormat::Y8: 						return AV_PIX_FMT_GRAY8;
		case ColorFormat::Y8A8: 					return AV_PIX_FMT_YA8;

		case ColorFormat::R8G8B8: 					return AV_PIX_FMT_RGB24;
		case ColorFormat::B8G8R8: 					return AV_PIX_FMT_BGR24;

		case ColorFormat::A8R8G8B8:					return AV_PIX_FMT_ARGB;
		case ColorFormat::A8B8G8R8:					return AV_PIX_FMT_ABGR;
		case ColorFormat::R8G8B8A8:					return AV_PIX_FMT_RGBA;
		case ColorFormat::B8G8R8A8:					return AV_PIX_FMT_BGRA;

		case ColorFormat::A8R8G8B8_32:				return AV_PIX_FMT_RGB32;
		case ColorFormat::A8B8G8R8_32:				return AV_PIX_FMT_BGR32;
		case ColorFormat::R8G8B8A8_32:				return AV_PIX_FMT_RGB32_1;
		case ColorFormat::B8G8R8A8_32:				return AV_PIX_FMT_BGR32_1;

		case ColorFormat::X8R8G8B8_32:				return AV_PIX_FMT_0RGB32;
		case ColorFormat::X8B8G8R8_32:				return AV_PIX_FMT_0BGR32;
		case ColorFormat::R8G8B8X8_32:				return AV_PIX_FMT_NE(RGB0, 0BGR);
		case ColorFormat::B8G8R8X8_32:				return AV_PIX_FMT_NE(BGR0, 0RGB);

		case ColorFormat::G8_B8_R8_A8:				return AV_PIX_FMT_GBRAP;
		case ColorFormat::G8_B8_R8:					return AV_PIX_FMT_GBRP;

		
		//10bpc
		case ColorFormat::Y10X6_16:					return AV_PIX_FMT_GRAY10;
		
		case ColorFormat::G10X6_B10X6_R10X6_A10X6_16: return AV_PIX_FMT_GBRAP10;
		case ColorFormat::G10X6_B10X6_R10X6_16:		return AV_PIX_FMT_GBRP10;


		//12bpc
		case ColorFormat::Y12X4_16:					return AV_PIX_FMT_GRAY12;
		
		case ColorFormat::G12X4_B12X4_R12X4_A12X4_16: return AV_PIX_FMT_GBRAP12;
		case ColorFormat::G12X4_B12X4_R12X4_16:		return AV_PIX_FMT_GBRP12;

		//16bpc
		case ColorFormat::Y16:						return AV_PIX_FMT_GRAY16;
		case ColorFormat::Y16A16:					return AV_PIX_FMT_YA16;

		case ColorFormat::R16G16B16:				return AV_PIX_FMT_RGB48;
		case ColorFormat::B16G16R16:				return AV_PIX_FMT_BGR48;

		case ColorFormat::R16G16B16A16:				return AV_PIX_FMT_RGBA64;
		case ColorFormat::B16G16R16A16:				return AV_PIX_FMT_BGRA64;

		case ColorFormat::G16_B16_R16_A16:			return AV_PIX_FMT_GBRAP16;
		case ColorFormat::G16_B16_R16:				return AV_PIX_FMT_GBRP16;

		//32bpc
		case ColorFormat::Y32f:  					return AV_PIX_FMT_GRAYF32;
		
		case ColorFormat::G32f_B32f_R32f_A32f:		return AV_PIX_FMT_GBRAPF32;
		case ColorFormat::G32f_B32f_R32f:			return AV_PIX_FMT_GBRPF32;
		
		default: break;
		}
	}

	return AV_PIX_FMT_NONE;
}

PixelFormat toFFmpeg(const PixelFormatConversion& fmt) {
	return static_cast<PixelFormat>(toFFmpegLUT(fmt));
}


static PixelFormatConversion fromFFmpegLUT(AVPixelFormat fmt) {
	/*
	* Comments have been obtained from:
	* libavutil/pixfmt.h
	* 
	* U = Cb => B
	* V = Cr => R
	*/

	switch(fmt) {
	case AV_PIX_FMT_YUV420P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_420, true };				//planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
	case AV_PIX_FMT_YUYV422:			return { ColorFormat::G8B8G8R8, ColorSubsampling::RB_422, true };	  			//packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
	case AV_PIX_FMT_RGB24:	    		return { ColorFormat::R8G8B8, ColorSubsampling::RB_444, false }; 				//packed RGB 8:8:8, 24bpp, RGBRGB...
	case AV_PIX_FMT_BGR24:	    		return { ColorFormat::B8G8R8, ColorSubsampling::RB_444, false };				//packed RGB 8:8:8, 24bpp, BGRBGR...
	case AV_PIX_FMT_YUV422P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_422, true };				//planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
	case AV_PIX_FMT_YUV444P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_444, true };				//planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
	case AV_PIX_FMT_YUV410P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_410, true };				//planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
	case AV_PIX_FMT_YUV411P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_411, true };				//planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
	case AV_PIX_FMT_GRAY8:	    		return { ColorFormat::Y8, ColorSubsampling::RB_444, false }; 					//       Y        ,  8bpp
	//case AV_PIX_FMT_MONOWHITE:		return {}; /*NOT SUPPORTED*/													//       Y        ,  1bpp, 0 is white, 1 is black, in each byte pixels are ordered from the msb to the lsb
	//case AV_PIX_FMT_MONOBLACK:		return {}; /*NOT SUPPORTED*/ 													//       Y        ,  1bpp, 0 is black, 1 is white, in each byte pixels are ordered from the msb to the lsb
	//case AV_PIX_FMT_PAL8:	 			return {}; /*NOT SUPPORTED*/ 													//8 bits with AV_PIX_FMT_RGB32 palette
	case AV_PIX_FMT_YUVJ420P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_420, true };				//planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV420P and setting color_range
	case AV_PIX_FMT_YUVJ422P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_422, true };				//planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV422P and setting color_range
	case AV_PIX_FMT_YUVJ444P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_444, true };				//planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV444P and setting color_range
	case AV_PIX_FMT_UYVY422:			return { ColorFormat::B8G8R8G8, ColorSubsampling::RB_422, true };				//packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
	//case AV_PIX_FMT_UYYVYY411:		return {}; /*NOT SUPPORTED*/  													//packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
	//case AV_PIX_FMT_BGR8:	    		return {}; /*NOT SUPPORTED*/													//packed RGB 3:3:2,  8bpp, (msb)2B 3G 3R(lsb)
	//case AV_PIX_FMT_BGR4:	    		return {}; /*NOT SUPPORTED*/													//packed RGB 1:2:1 bitstream,  4bpp, (msb)1B 2G 1R(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
	//case AV_PIX_FMT_BGR4_BYTE:		return {}; /*NOT SUPPORTED*/													//packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
	//case AV_PIX_FMT_RGB8:	    		return {}; /*NOT SUPPORTED*/													//packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
	//case AV_PIX_FMT_RGB4:	    		return {}; /*NOT SUPPORTED*/													//packed RGB 1:2:1 bitstream,  4bpp, (msb)1R 2G 1B(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
	//case AV_PIX_FMT_RGB4_BYTE:		return {}; /*NOT SUPPORTED*/													//packed RGB 1:2:1,  8bpp, (msb)1R 2G 1B(lsb)
	case AV_PIX_FMT_NV12:	    		return { ColorFormat::G8_B8R8, ColorSubsampling::RB_420, true };  				//planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
	case AV_PIX_FMT_NV21:	    		return { ColorFormat::G8_R8B8, ColorSubsampling::RB_420, true };				//as above, but U and V bytes are swapped

	case AV_PIX_FMT_ARGB:				return { ColorFormat::A8R8G8B8, ColorSubsampling::RB_444, false };				//packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
	case AV_PIX_FMT_RGBA:				return { ColorFormat::R8G8B8A8, ColorSubsampling::RB_444, false };				//packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
	case AV_PIX_FMT_ABGR:				return { ColorFormat::A8B8G8R8, ColorSubsampling::RB_444, false };				//packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
	case AV_PIX_FMT_BGRA:				return { ColorFormat::B8G8R8A8, ColorSubsampling::RB_444, false };				//packed BGRA 8:8:8:8, 32bpp, BGRABGRA...

	case AV_PIX_FMT_GRAY16:				return { ColorFormat::Y16, ColorSubsampling::RB_444, false };					//       Y        , 16bpp
	case AV_PIX_FMT_YUV440P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_440, true };				//planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
	case AV_PIX_FMT_YUVJ440P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_440, true };				//planar YUV 4:4:0 full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV440P and setting color_range
	case AV_PIX_FMT_YUVA420P:			return { ColorFormat::G8_B8_R8_A8, ColorSubsampling::RB_420, true };			//planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)

	case AV_PIX_FMT_RGB48: 				return { ColorFormat::R16G16B16, ColorSubsampling::RB_444, false }; 			//packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B
	case AV_PIX_FMT_RGB565: 			return { ColorFormat::R5G6B5_16, ColorSubsampling::RB_444, false }; 			//packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb)
	case AV_PIX_FMT_RGB555:				return { ColorFormat::X1R5G5B5_16, ColorSubsampling::RB_444, false }; 			//packed RGB 5:5:5, 16bpp, (msb)1X 5R 5G 5B(lsb), X=unused/undefined

	case AV_PIX_FMT_BGR48:   			return { ColorFormat::B16G16R16, ColorSubsampling::RB_444, false }; 			//packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B
	case AV_PIX_FMT_BGR565:				return { ColorFormat::B5G6R5_16, ColorSubsampling::RB_444, false }; 			//packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb)
	case AV_PIX_FMT_BGR555:				return { ColorFormat::X1B5G5R5_16, ColorSubsampling::RB_444, false }; 			//packed BGR 5:5:5, 16bpp, (msb)1X 5B 5G 5R(lsb), X=unused/undefined

	case AV_PIX_FMT_YUV420P16: 			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_420, true }; 			//planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples)
	case AV_PIX_FMT_YUV422P16: 			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_422, true }; 			//planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples)
	case AV_PIX_FMT_YUV444P16: 			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_444, true }; 			//planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples)
	//case AV_PIX_FMT_DXVA2_VLD: 		return {}; /*NOT SUPPORTED*/   													//HW decoding through DXVA2, Picture.data[3] contains a LPDIRECT3DSURFACE9 pointer

	case AV_PIX_FMT_RGB444:				return { ColorFormat::X4R4G4B4_16, ColorSubsampling::RB_444, false }; 			//packed RGB 4:4:4, 16bpp, (msb)4X 4R 4G 4B(lsb), X=unused/undefined
	case AV_PIX_FMT_BGR444: 			return { ColorFormat::X4B4G4R4_16, ColorSubsampling::RB_444, false }; 			//packed BGR 4:4:4, 16bpp, (msb)4X 4B 4G 4R(lsb), X=unused/undefined
	case AV_PIX_FMT_YA8:				return { ColorFormat::Y8A8, ColorSubsampling::RB_444, false }; 					//8 bits gray, 8 bits alpha

	/**
	 * The following 12 formats have the disadvantage of needing 1 format for each bit depth.
	 * Notice that each 9/10 bits sample is stored in 16 bits with extra padding.
	 * If you want to support multiple bit depths, then using AV_PIX_FMT_YUV420P16* with the bpp stored separately is better.
	 */
	case AV_PIX_FMT_YUV420P9:			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_420, true };	//planar YUV 4:2:0, 13.5bpp, (1 Cr & Cb sample per 2x2 Y samples)
	case AV_PIX_FMT_YUV420P10:			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_420, true };	//planar YUV 4:2:0, 15bpp, (1 Cr & Cb sample per 2x2 Y samples)
	case AV_PIX_FMT_YUV422P10:			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_422, true };	//planar YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples)
	case AV_PIX_FMT_YUV444P9:			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_444, true };	//planar YUV 4:4:4, 27bpp, (1 Cr & Cb sample per 1x1 Y samples)
	case AV_PIX_FMT_YUV444P10:			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_444, true };	//planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples)
	case AV_PIX_FMT_YUV422P9:			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_422, true };	//planar YUV 4:2:2, 18bpp, (1 Cr & Cb sample per 2x1 Y samples)
	case AV_PIX_FMT_GBRP:				return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_444, false }; 				//planar GBR 4:4:4 24bpp
	case AV_PIX_FMT_GBRP9:				return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_444, false };	//planar GBR 4:4:4 27bpp
	case AV_PIX_FMT_GBRP10:				return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_444, false };	//planar GBR 4:4:4 30bpp
	case AV_PIX_FMT_GBRP16:				return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_444, false };			//planar GBR 4:4:4 48bpp
	case AV_PIX_FMT_YUVA422P:			return { ColorFormat::G8_B8_R8_A8, ColorSubsampling::RB_422, true };			//planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
	case AV_PIX_FMT_YUVA444P:			return { ColorFormat::G8_B8_R8_A8, ColorSubsampling::RB_444, true };			//planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
	case AV_PIX_FMT_YUVA420P9:			return { ColorFormat::G10X6_B10X6_R10X6_A10X6_16, ColorSubsampling::RB_420, true }; //planar YUV 4:2:0 22.5bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
	case AV_PIX_FMT_YUVA422P9:			return { ColorFormat::G10X6_B10X6_R10X6_A10X6_16, ColorSubsampling::RB_422, true }; //planar YUV 4:2:2 27bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
	case AV_PIX_FMT_YUVA444P9:			return { ColorFormat::G10X6_B10X6_R10X6_A10X6_16, ColorSubsampling::RB_444, true }; //planar YUV 4:4:4 36bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
	case AV_PIX_FMT_YUVA420P10:			return { ColorFormat::G10X6_B10X6_R10X6_A10X6_16, ColorSubsampling::RB_420, true }; //planar YUV 4:2:0 25bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
	case AV_PIX_FMT_YUVA422P10:			return { ColorFormat::G10X6_B10X6_R10X6_A10X6_16, ColorSubsampling::RB_422, true }; //planar YUV 4:2:2 30bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
	case AV_PIX_FMT_YUVA444P10:			return { ColorFormat::G10X6_B10X6_R10X6_A10X6_16, ColorSubsampling::RB_444, true }; //planar YUV 4:4:4 40bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
	case AV_PIX_FMT_YUVA420P16:			return { ColorFormat::G16_B16_R16_A16, ColorSubsampling::RB_420, true };		//planar YUV 4:2:0 40bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
	case AV_PIX_FMT_YUVA422P16:			return { ColorFormat::G16_B16_R16_A16, ColorSubsampling::RB_422, true };		//planar YUV 4:2:2 48bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
	case AV_PIX_FMT_YUVA444P16:			return { ColorFormat::G16_B16_R16_A16, ColorSubsampling::RB_444, true };		//planar YUV 4:4:4 64bpp, (1 Cr & Cb sample per 1x1 Y & A samples)

	//case AV_PIX_FMT_VDPAU: 			return {}; /*NOT SUPPORTED*/     												//HW acceleration through VDPAU, Picture.data[3] contains a VdpVideoSurface

	//case AV_PIX_FMT_XYZ12:			return {}; /*NOT SUPPORTED*/  													//packed XYZ 4:4:4, 36 bpp, (msb) 12X, 12Y, 12Z (lsb), the 2-byte value for each X/Y/Z, the 4 lower bits are set to 0


	case AV_PIX_FMT_NV16: 				return { ColorFormat::G8_B8R8, ColorSubsampling::RB_422, true };	 			//interleaved chroma YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
	case AV_PIX_FMT_NV20:      			return { ColorFormat::G10X6_B10X6R10X6_16, ColorSubsampling::RB_422, true };	//interleaved chroma YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples)

	case AV_PIX_FMT_RGBA64:     		return { ColorFormat::R16G16B16A16, ColorSubsampling::RB_444, false };			//packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component
	case AV_PIX_FMT_BGRA64:    			return { ColorFormat::B16G16R16A16, ColorSubsampling::RB_444, false };			//packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component

	case AV_PIX_FMT_YVYU422:			return { ColorFormat::G8R8G8B8, ColorSubsampling::RB_422, true };	 			//packed YUV 4:2:2, 16bpp, Y0 Cr Y1 Cb

	case AV_PIX_FMT_YA16:      			return { ColorFormat::Y16A16, ColorSubsampling::RB_444, false }; 				//16 bits gray, 16 bits alpha

	case AV_PIX_FMT_GBRAP:  			return { ColorFormat::G8_B8_R8_A8, ColorSubsampling::RB_444, false };			//planar GBRA 4:4:4:4 32bpp
	case AV_PIX_FMT_GBRAP16:  			return { ColorFormat::G16_B16_R16_A16, ColorSubsampling::RB_444, false };		//planar GBRA 4:4:4:4 64bpp

	//case AV_PIX_FMT_QSV: 				return {}; /*NOT SUPPORTED*/ 													//HW acceleration through QSV, data[3] contains a pointer to the mfxFrameSurface1 structure.
	//case AV_PIX_FMT_MMAL: 			return {}; /*NOT SUPPORTED*/ 													//HW acceleration though MMAL, data[3] contains a pointer to the MMAL_BUFFER_HEADER_T structure.
	//case AV_PIX_FMT_D3D11VA_VLD: 		return {}; /*NOT SUPPORTED*/ 													//HW decoding through Direct3D11 via old API, Picture.data[3] contains a ID3D11VideoDecoderOutputView pointer
	//case AV_PIX_FMT_CUDA: 			return {}; /*NOT SUPPORTED*/

	case AV_PIX_FMT_0RGB: 				return { Utils::bele(ColorFormat::X8R8G8B8_32, ColorFormat::B8G8R8X8_32), ColorSubsampling::RB_444, false };  //packed RGB 8:8:8, 32bpp, XRGBXRGB...   X=unused/undefined
	case AV_PIX_FMT_RGB0: 				return { Utils::bele(ColorFormat::R8G8B8X8_32, ColorFormat::X8B8G8R8_32), ColorSubsampling::RB_444, false };  //packed RGB 8:8:8, 32bpp, RGBXRGBX...   X=unused/undefined
	case AV_PIX_FMT_0BGR: 				return { Utils::bele(ColorFormat::X8B8G8R8_32, ColorFormat::R8G8B8X8_32), ColorSubsampling::RB_444, false };  //packed BGR 8:8:8, 32bpp, XBGRXBGR...   X=unused/undefined
	case AV_PIX_FMT_BGR0: 				return { Utils::bele(ColorFormat::B8G8R8X8_32, ColorFormat::X8R8G8B8_32), ColorSubsampling::RB_444, false };  //packed BGR 8:8:8, 32bpp, BGRXBGRX...   X=unused/undefined

	case AV_PIX_FMT_YUV420P12: 			return { ColorFormat::G12X4_B12X4_R12X4_16, ColorSubsampling::RB_420, true };	//planar YUV 4:2:0,18bpp, (1 Cr & Cb sample per 2x2 Y samples)
	case AV_PIX_FMT_YUV420P14: 			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_420, true };			//planar YUV 4:2:0,21bpp, (1 Cr & Cb sample per 2x2 Y samples)
	case AV_PIX_FMT_YUV422P12: 			return { ColorFormat::G12X4_B12X4_R12X4_16, ColorSubsampling::RB_422, true };	//planar YUV 4:2:2,24bpp, (1 Cr & Cb sample per 2x1 Y samples)
	case AV_PIX_FMT_YUV422P14: 			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_422, true };			//planar YUV 4:2:2,28bpp, (1 Cr & Cb sample per 2x1 Y samples)
	case AV_PIX_FMT_YUV444P12: 			return { ColorFormat::G12X4_B12X4_R12X4_16, ColorSubsampling::RB_444, true };	//planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples)
	case AV_PIX_FMT_YUV444P14: 			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_444, true };			//planar YUV 4:4:4,42bpp, (1 Cr & Cb sample per 1x1 Y samples)
	case AV_PIX_FMT_GBRP12:    			return { ColorFormat::G12X4_B12X4_R12X4_16, ColorSubsampling::RB_444, false };	//planar GBR 4:4:4 36bpp
	case AV_PIX_FMT_GBRP14:    			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_444, false };			//planar GBR 4:4:4 42bpp
	case AV_PIX_FMT_YUVJ411P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_411, true };	    		//planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples) full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV411P and setting color_range

	//case AV_PIX_FMT_BAYER_BGGR8:  	return {}; /*NOT SUPPORTED*/ 													//bayer, BGBG..(odd line), GRGR..(even line), 8-bit samples
	//case AV_PIX_FMT_BAYER_RGGB8:  	return {}; /*NOT SUPPORTED*/ 													//bayer, RGRG..(odd line), GBGB..(even line), 8-bit samples
	//case AV_PIX_FMT_BAYER_GBRG8:  	return {}; /*NOT SUPPORTED*/ 													//bayer, GBGB..(odd line), RGRG..(even line), 8-bit samples
	//case AV_PIX_FMT_BAYER_GRBG8:  	return {}; /*NOT SUPPORTED*/ 													//bayer, GRGR..(odd line), BGBG..(even line), 8-bit samples
	//case AV_PIX_FMT_BAYER_BGGR16:		return {}; /*NOT SUPPORTED*/ 													//bayer, BGBG..(odd line), GRGR..(even line), 16-bit samples
	//case AV_PIX_FMT_BAYER_RGGB16:		return {}; /*NOT SUPPORTED*/ 													//bayer, RGRG..(odd line), GBGB..(even line), 16-bit samples
	//case AV_PIX_FMT_BAYER_GBRG16:		return {}; /*NOT SUPPORTED*/ 													//bayer, GBGB..(odd line), RGRG..(even line), 16-bit samples
	//case AV_PIX_FMT_BAYER_GRBG16:		return {}; /*NOT SUPPORTED*/ 													//bayer, GRGR..(odd line), BGBG..(even line), 16-bit samples

	//case AV_PIX_FMT_XVMC:				return {}; /*NOT SUPPORTED*/													//XVideo Motion Acceleration via common packet passing

	case AV_PIX_FMT_YUV440P10: 			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_440, true };	//planar YUV 4:4:0,20bpp, (1 Cr & Cb sample per 1x2 Y samples)
	case AV_PIX_FMT_YUV440P12: 			return { ColorFormat::G12X4_B12X4_R12X4_16, ColorSubsampling::RB_440, true };	//planar YUV 4:4:0,24bpp, (1 Cr & Cb sample per 1x2 Y samples)
	case AV_PIX_FMT_AYUV64:   			return { ColorFormat::A16G16B16R16, ColorSubsampling::RB_444, true };			//packed AYUV 4:4:4,64bpp (1 Cr & Cb sample per 1x1 Y & A samples)

	//case AV_PIX_FMT_VIDEOTOOLBOX:		return {}; /*NOT SUPPORTED*/													//hardware decoding through Videotoolbox

	case AV_PIX_FMT_P010: 				return { ColorFormat::G10X6_B10X6R10X6_16, ColorSubsampling::RB_420, true };	//like NV12, with 10bpp per component, data in the high bits, zeros in the low bits

	case AV_PIX_FMT_GBRAP12:  			return { ColorFormat::G12X4_B12X4_R12X4_A12X4_16, ColorSubsampling::RB_444, false }; //planar GBR 4:4:4:4 48bpp
	case AV_PIX_FMT_GBRAP10:  			return { ColorFormat::G10X6_B10X6_R10X6_A10X6_16, ColorSubsampling::RB_444, false }; //planar GBR 4:4:4:4 40bpp

	//case AV_PIX_FMT_MEDIACODEC: 		return {}; /*NOT SUPPORTED*/													//hardware decoding through MediaCodec

	case AV_PIX_FMT_GRAY12:   			return { ColorFormat::Y12X4_16, ColorSubsampling::RB_444, false };				//       Y        , 12bpp
	case AV_PIX_FMT_GRAY10:   			return { ColorFormat::Y10X6_16, ColorSubsampling::RB_444, false };				//       Y        , 10bpp

	case AV_PIX_FMT_P016:  				return { ColorFormat::G16_B16R16, ColorSubsampling::RB_420, true };				//like NV12, with 16bpp per component

	//case AV_PIX_FMT_D3D11:			return {}; /*NOT SUPPORTED*/													//Hardware surfaces for Direct3D11.

	case AV_PIX_FMT_GRAY9:				return { ColorFormat::Y10X6_16, ColorSubsampling::RB_444, false };				//       Y        , 9bpp

	case AV_PIX_FMT_GBRPF32:  			return { ColorFormat::G32f_B32f_R32f, ColorSubsampling::RB_444, false };		//IEEE-754 single precision planar GBR 4:4:4,     96bpp
	case AV_PIX_FMT_GBRAPF32: 			return { ColorFormat::G32f_B32f_R32f_A32f, ColorSubsampling::RB_444, false };	//IEEE-754 single precision planar GBRA 4:4:4:4, 128bpp

	//case AV_PIX_FMT_DRM_PRIME:		return {}; /*NOT SUPPORTED*/													//DRM-managed buffers exposed through PRIME buffer sharing.

	//case AV_PIX_FMT_OPENCL:			return {}; /*NOT SUPPORTED*/													//Hardware surfaces for OpenCL.

	case AV_PIX_FMT_GRAY14:   			return { ColorFormat::Y16, ColorSubsampling::RB_444, false };					//       Y        , 14bpp

	case AV_PIX_FMT_GRAYF32:  			return { ColorFormat::Y32f, ColorSubsampling::RB_444, false };					//IEEE-754 single precision Y, 32bpp

	case AV_PIX_FMT_YUVA422P12: 		return { ColorFormat::G12X4_B12X4_R12X4_A12X4_16, ColorSubsampling::RB_422, true }; //planar YUV 4:2:2,24bpp, (1 Cr & Cb sample per 2x1 Y samples), 12b alpha
	case AV_PIX_FMT_YUVA444P12: 		return { ColorFormat::G12X4_B12X4_R12X4_A12X4_16, ColorSubsampling::RB_444, true }; //planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples), 12b alpha

	case AV_PIX_FMT_NV24:      			return { ColorFormat::G8_B8R8, ColorSubsampling::RB_444, true };				//planar YUV 4:4:4, 24bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
	case AV_PIX_FMT_NV42:     			return { ColorFormat::G8_R8B8, ColorSubsampling::RB_444, true };				//as above, but U and V bytes are swapped

	//case AV_PIX_FMT_VULKAN:			return {}; /*NOT SUPPORTED*/													//Vulkan hardware images.

	case AV_PIX_FMT_Y210:   			return { ColorFormat::G10X6B10X6G10X6R10X6_16, ColorSubsampling::RB_444, true };//packed YUV 4:2:2 like YUYV422, 20bpp, data in the high bits


	default: 							return { ColorFormat::NONE, ColorSubsampling::NONE, false };
	}
}

PixelFormatConversion fromFFmpeg(PixelFormat fmt) {
	return fromFFmpegLUT(static_cast<AVPixelFormat>(fmt));
}

}
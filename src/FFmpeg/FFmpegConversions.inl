#include "FFmpegConversions.h"

namespace Zuazo::FFmpeg {

constexpr AVRational toFFmpeg(Rational rat) {
	return AVRational {
		rat.getNumerator(), 
		rat.getDenominator() 
	};
}

constexpr Rational fromFFmpeg(AVRational rat) {
	return Rational(rat.num, rat.den);
}



constexpr AVPixelFormat toFFmpeg(ColorFormat fmt, ColorSubsampling subs, ColorModel model) {
	if(isYCbCr(model)) {
		switch(fmt) {
		//8bpc
		case ColorFormat::B8G8R8G8:					return AV_PIX_FMT_UYVY422;
		case ColorFormat::G8R8G8B8:					return AV_PIX_FMT_YVYU422;
		case ColorFormat::G8B8G8R8:					return AV_PIX_FMT_YUYV422;

		case ColorFormat::G8_B8_R8:
			switch (subs) {
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
			switch (subs) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_NV24;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_NV16;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_NV12;
			default: break;
			}
			break;

		case ColorFormat::G8_R8B8:
			switch (subs) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_NV42;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_NV21;
			default: break;
			}
			break;


		//10bpc
		case ColorFormat::G10X6B10X6G10X6R10X6_16:	return AV_PIX_FMT_Y210;
		
		case ColorFormat::G10X6_B10X6_R10X6_16:
			switch (subs) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_YUV444P10;
			case ColorSubsampling::RB_440:			return AV_PIX_FMT_YUV440P10;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YUV422P10;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_YUV420P10;
			default: break;
			}
			break;

		case ColorFormat::G10X6_B10X6R10X6_16:
			switch (subs) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_NV20;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_P010;
			default: break;
			}
			break;


		//12bpc	
		case ColorFormat::G12X4_B12X4_R12X4_16:
			switch (subs) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_YUV444P12;
			case ColorSubsampling::RB_440:			return AV_PIX_FMT_YUV440P12;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YUV422P12;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_YUV420P12;
			default: break;
			}
			break;


		//16bpc
		case ColorFormat::G16_B16_R16:
			switch (subs) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_YUV444P16;
			case ColorSubsampling::RB_422:			return AV_PIX_FMT_YUV422P16;
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_YUV420P16;
			default: break;
			}
			break;

		case ColorFormat::G16_B16R16:
			switch (subs) {
			case ColorSubsampling::RB_420:			return AV_PIX_FMT_P016;
			default: break;
			}
			break;


		default: break;
		}
	} else {
		//RGB
		switch(fmt) {
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

		case ColorFormat::G8_B8_R8:
			switch (subs) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_GBRP;
			default: break;
			}
			break;

		
		//10bpc
		case ColorFormat::Y10X6_16:					return AV_PIX_FMT_GRAY10;
		
		case ColorFormat::G10X6_B10X6_R10X6_16:
			switch (subs) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_GBRP10;
			default: break;
			}
			break;


		//12bpc
		case ColorFormat::Y12X4_16:					return AV_PIX_FMT_GRAY12;
		
		case ColorFormat::G12X4_B12X4_R12X4_16:
			switch (subs) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_GBRP12;
			default: break;
			}
			break;


		//16bpc
		case ColorFormat::Y16:						return AV_PIX_FMT_GRAY16;
		case ColorFormat::Y16A16:					return AV_PIX_FMT_YA16;

		case ColorFormat::R16G16B16:				return AV_PIX_FMT_RGB48;
		case ColorFormat::B16G16R16:				return AV_PIX_FMT_BGR48;

		case ColorFormat::R16G16B16A16:				return AV_PIX_FMT_RGBA64;
		case ColorFormat::B16G16R16A16:				return AV_PIX_FMT_BGRA64;

		case ColorFormat::G16_B16_R16:
			switch (subs) {
			case ColorSubsampling::RB_444:			return AV_PIX_FMT_GBRP16;
			default: break;
			}
			break;

		//32bpc
		case ColorFormat::Y32f:  					return AV_PIX_FMT_GRAYF32;

		default: break;
		}
	}

	return AV_PIX_FMT_NONE;
}

constexpr std::tuple<ColorFormat, ColorSubsampling> fromFFmpeg(AVPixelFormat fmt) {
	/*
	 * Format comments have been obtained from:
	 * libavutil/pixfmt.h
	 * 
	 * U = Cb => B
	 * V = Cr => R
	 */

	switch(fmt) {
	case AV_PIX_FMT_YUV420P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_420 };				//planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
	case AV_PIX_FMT_YUYV422:			return { ColorFormat::G8B8G8R8, ColorSubsampling::RB_422 };	  			//packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
	case AV_PIX_FMT_RGB24:	    		return { ColorFormat::R8G8B8, ColorSubsampling::RB_444 }; 				//packed RGB 8:8:8, 24bpp, RGBRGB...
	case AV_PIX_FMT_BGR24:	    		return { ColorFormat::B8G8R8, ColorSubsampling::RB_444 };				//packed RGB 8:8:8, 24bpp, BGRBGR...
	case AV_PIX_FMT_YUV422P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_422 };				//planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
	case AV_PIX_FMT_YUV444P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_444 };				//planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
	case AV_PIX_FMT_YUV410P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_410 };				//planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
	case AV_PIX_FMT_YUV411P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_411 };				//planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
	case AV_PIX_FMT_GRAY8:	    		return { ColorFormat::Y8, ColorSubsampling::RB_444 }; 					//       Y        ,  8bpp
	//case AV_PIX_FMT_MONOWHITE:		return {}; /* NOT SUPPORTED */											//       Y        ,  1bpp, 0 is white, 1 is black, in each byte pixels are ordered from the msb to the lsb
	//case AV_PIX_FMT_MONOBLACK:		return {}; /* NOT SUPPORTED */ 											//       Y        ,  1bpp, 0 is black, 1 is white, in each byte pixels are ordered from the msb to the lsb
	//case AV_PIX_FMT_PAL8:	 			return {}; /* NOT SUPPORTED */ 											//8 bits with AV_PIX_FMT_RGB32 palette
	//case AV_PIX_FMT_YUVJ420P:			return {}; /* DEPRECATED */ 											//planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV420P and setting color_range
	//case AV_PIX_FMT_YUVJ422P:			return {}; /* DEPRECATED */ 											//planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV422P and setting color_range
	//case AV_PIX_FMT_YUVJ444P:			return {}; /* DEPRECATED */ 											//planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV444P and setting color_range
	case AV_PIX_FMT_UYVY422:			return { ColorFormat::B8G8R8G8, ColorSubsampling::RB_422 };				//packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
	//case AV_PIX_FMT_UYYVYY411:		return {}; /* NOT SUPPORTED */  										//packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
	//case AV_PIX_FMT_BGR8:	    		return {}; /* NOT SUPPORTED */											//packed RGB 3:3:2,  8bpp, (msb)2B 3G 3R(lsb)
	//case AV_PIX_FMT_BGR4:	    		return {}; /* NOT SUPPORTED */											//packed RGB 1:2:1 bitstream,  4bpp, (msb)1B 2G 1R(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
	//case AV_PIX_FMT_BGR4_BYTE:		return {}; /* NOT SUPPORTED */											//packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
	//case AV_PIX_FMT_RGB8:	    		return {}; /* NOT SUPPORTED */											//packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
	//case AV_PIX_FMT_RGB4:	    		return {}; /* NOT SUPPORTED */											//packed RGB 1:2:1 bitstream,  4bpp, (msb)1R 2G 1B(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
	//case AV_PIX_FMT_RGB4_BYTE:		return {}; /* NOT SUPPORTED */											//packed RGB 1:2:1,  8bpp, (msb)1R 2G 1B(lsb)
	case AV_PIX_FMT_NV12:	    		return { ColorFormat::G8_B8R8, ColorSubsampling::RB_420 };  			//planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
	case AV_PIX_FMT_NV21:	    		return { ColorFormat::G8_R8B8, ColorSubsampling::RB_420 };				//as above, but U and V bytes are swapped

    case AV_PIX_FMT_ARGB:				return { ColorFormat::A8R8G8B8, ColorSubsampling::RB_444 };				//packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
    case AV_PIX_FMT_RGBA:				return { ColorFormat::R8G8B8A8, ColorSubsampling::RB_444 };				//packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
    case AV_PIX_FMT_ABGR:				return { ColorFormat::A8B8G8R8, ColorSubsampling::RB_444 };				//packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
    case AV_PIX_FMT_BGRA:				return { ColorFormat::B8G8R8A8, ColorSubsampling::RB_444 };				//packed BGRA 8:8:8:8, 32bpp, BGRABGRA...

    case AV_PIX_FMT_GRAY16:				return { ColorFormat::Y16, ColorSubsampling::RB_444 };					//       Y        , 16bpp
    case AV_PIX_FMT_YUV440P:			return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_440 };				//planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
    //case AV_PIX_FMT_YUVJ440P:			return {}; /* DEPRECATED */ 											//planar YUV 4:4:0 full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV440P and setting color_range
    //case AV_PIX_FMT_YUVA420P:			return {}; /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)

    case AV_PIX_FMT_RGB48: 				return { ColorFormat::R16G16B16, ColorSubsampling::RB_444 }; 			//packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B
    case AV_PIX_FMT_RGB565: 			return { ColorFormat::R5G6B5_16, ColorSubsampling::RB_444 }; 			//packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb)
    case AV_PIX_FMT_RGB555:				return { ColorFormat::X1R5G5B5_16, ColorSubsampling::RB_444 }; 			//packed RGB 5:5:5, 16bpp, (msb)1X 5R 5G 5B(lsb), X=unused/undefined

	case AV_PIX_FMT_BGR48:   			return { ColorFormat::B16G16R16, ColorSubsampling::RB_444 }; 			//packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B
    case AV_PIX_FMT_BGR565:				return { ColorFormat::B5G6R5_16, ColorSubsampling::RB_444 }; 			//packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb)
    case AV_PIX_FMT_BGR555:				return { ColorFormat::X1B5G5R5_16, ColorSubsampling::RB_444 }; 			//packed BGR 5:5:5, 16bpp, (msb)1X 5B 5G 5R(lsb), X=unused/undefined

    case AV_PIX_FMT_YUV420P16: 			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_420 }; 			//planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples)
    case AV_PIX_FMT_YUV422P16: 			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_422 }; 			//planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples)
    case AV_PIX_FMT_YUV444P16: 			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_444 }; 			//planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples)
    //case AV_PIX_FMT_DXVA2_VLD: 		return {}; /* NOT SUPPORTED */   										//HW decoding through DXVA2, Picture.data[3] contains a LPDIRECT3DSURFACE9 pointer

    case AV_PIX_FMT_RGB444:				return { ColorFormat::X4R4G4B4_16, ColorSubsampling::RB_444 }; 			//packed RGB 4:4:4, 16bpp, (msb)4X 4R 4G 4B(lsb), X=unused/undefined
    case AV_PIX_FMT_BGR444: 			return { ColorFormat::X4B4G4R4_16, ColorSubsampling::RB_444 }; 			//packed BGR 4:4:4, 16bpp, (msb)4X 4B 4G 4R(lsb), X=unused/undefined
    case AV_PIX_FMT_YA8:				return { ColorFormat::Y8A8, ColorSubsampling::RB_444 }; 				//8 bits gray, 8 bits alpha

    /**
     * The following 12 formats have the disadvantage of needing 1 format for each bit depth.
     * Notice that each 9/10 bits sample is stored in 16 bits with extra padding.
     * If you want to support multiple bit depths, then using AV_PIX_FMT_YUV420P16* with the bpp stored separately is better.
     */
    case AV_PIX_FMT_YUV420P9:			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_420 };	//planar YUV 4:2:0, 13.5bpp, (1 Cr & Cb sample per 2x2 Y samples)
    case AV_PIX_FMT_YUV420P10:			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_420 };	//planar YUV 4:2:0, 15bpp, (1 Cr & Cb sample per 2x2 Y samples)
    case AV_PIX_FMT_YUV422P10:			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_422 };	//planar YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples)
    case AV_PIX_FMT_YUV444P9:			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_444 };	//planar YUV 4:4:4, 27bpp, (1 Cr & Cb sample per 1x1 Y samples)
    case AV_PIX_FMT_YUV444P10:			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_444 };	//planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples)
    case AV_PIX_FMT_YUV422P9:			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_422 };	//planar YUV 4:2:2, 18bpp, (1 Cr & Cb sample per 2x1 Y samples)
    case AV_PIX_FMT_GBRP:				return { ColorFormat::G8_B8_R8, ColorSubsampling::RB_444 }; 			//planar GBR 4:4:4 24bpp
    case AV_PIX_FMT_GBRP9:				return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_444 };	//planar GBR 4:4:4 27bpp
    case AV_PIX_FMT_GBRP10:				return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_444 };	//planar GBR 4:4:4 30bpp
    case AV_PIX_FMT_GBRP16:				return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_444 };			//planar GBR 4:4:4 48bpp
    //case AV_PIX_FMT_YUVA422P:			return {}; /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
	//case AV_PIX_FMT_YUVA444P:			return {}; /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
    //case AV_PIX_FMT_YUVA420P9:		return {}; /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:2:0 22.5bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
    //case AV_PIX_FMT_YUVA422P9:		return {}; /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:2:2 27bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
    //case AV_PIX_FMT_YUVA444P9:		return {}; /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:4:4 36bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
    //case AV_PIX_FMT_YUVA420P10:		return	{} /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:2:0 25bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
    //case AV_PIX_FMT_YUVA422P10:		return	{} /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:2:2 30bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
    //case AV_PIX_FMT_YUVA444P10:		return	{} /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:4:4 40bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
    //case AV_PIX_FMT_YUVA420P16:		return	{} /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:2:0 40bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
    //case AV_PIX_FMT_YUVA422P16:		return	{} /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:2:2 48bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
    //case AV_PIX_FMT_YUVA444P16:		return	{} /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:4:4 64bpp, (1 Cr & Cb sample per 1x1 Y & A samples)

	//case AV_PIX_FMT_VDPAU: 			return {}; /* NOT SUPPORTED */     										//HW acceleration through VDPAU, Picture.data[3] contains a VdpVideoSurface

	//case AV_PIX_FMT_XYZ12:			return {}; /* NOT SUPPORTED */  										//packed XYZ 4:4:4, 36 bpp, (msb) 12X, 12Y, 12Z (lsb), the 2-byte value for each X/Y/Z, the 4 lower bits are set to 0


    case AV_PIX_FMT_NV16: 				return { ColorFormat::G8_B8R8, ColorSubsampling::RB_422 };	 			//interleaved chroma YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    case AV_PIX_FMT_NV20:      			return { ColorFormat::G10X6_B10X6R10X6_16, ColorSubsampling::RB_422 };	//interleaved chroma YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples)

    case AV_PIX_FMT_RGBA64:     		return { ColorFormat::R16G16B16A16, ColorSubsampling::RB_444 };			//packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component
    case AV_PIX_FMT_BGRA64:    			return { ColorFormat::B16G16R16A16, ColorSubsampling::RB_444 };			//packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component

    case AV_PIX_FMT_YVYU422:			return { ColorFormat::G8R8G8B8, ColorSubsampling::RB_422 };	 			//packed YUV 4:2:2, 16bpp, Y0 Cr Y1 Cb

    case AV_PIX_FMT_YA16:      			return { ColorFormat::Y16A16, ColorSubsampling::RB_444 }; 				//16 bits gray, 16 bits alpha

    //case AV_PIX_FMT_GBRAP:  			return	{} /* NOT SUPPORTED TODO: Add support*/							//planar GBRA 4:4:4:4 32bpp
    //case AV_PIX_FMT_GBRAP16:  		return	{} /* NOT SUPPORTED TODO: Add support*/							//planar GBRA 4:4:4:4 64bpp

	//case AV_PIX_FMT_QSV: 				return {}; /* NOT SUPPORTED */ 											//HW acceleration through QSV, data[3] contains a pointer to the mfxFrameSurface1 structure.
	//case AV_PIX_FMT_MMAL: 			return {}; /* NOT SUPPORTED */ 											//HW acceleration though MMAL, data[3] contains a pointer to the MMAL_BUFFER_HEADER_T structure.
	//case AV_PIX_FMT_D3D11VA_VLD: 		return {}; /* NOT SUPPORTED */ 											//HW decoding through Direct3D11 via old API, Picture.data[3] contains a ID3D11VideoDecoderOutputView pointer
	//case AV_PIX_FMT_CUDA: 			return {}; /* NOT SUPPORTED */

    case AV_PIX_FMT_0RGB: 				return { ZUAZO_BE_LE(ColorFormat::X8R8G8B8_32, ColorFormat::B8G8R8X8_32), ColorSubsampling::RB_444 };  //packed RGB 8:8:8, 32bpp, XRGBXRGB...   X=unused/undefined
    case AV_PIX_FMT_RGB0: 				return { ZUAZO_BE_LE(ColorFormat::R8G8B8X8_32, ColorFormat::X8B8G8R8_32), ColorSubsampling::RB_444 };  //packed RGB 8:8:8, 32bpp, RGBXRGBX...   X=unused/undefined
    case AV_PIX_FMT_0BGR: 				return { ZUAZO_BE_LE(ColorFormat::X8B8G8R8_32, ColorFormat::R8G8B8X8_32), ColorSubsampling::RB_444 };  //packed BGR 8:8:8, 32bpp, XBGRXBGR...   X=unused/undefined
    case AV_PIX_FMT_BGR0: 				return { ZUAZO_BE_LE(ColorFormat::B8G8R8X8_32, ColorFormat::X8R8G8B8_32), ColorSubsampling::RB_444 };  //packed BGR 8:8:8, 32bpp, BGRXBGRX...   X=unused/undefined

    case AV_PIX_FMT_YUV420P12: 			return { ColorFormat::G12X4_B12X4_R12X4_16, ColorSubsampling::RB_420 };	//planar YUV 4:2:0,18bpp, (1 Cr & Cb sample per 2x2 Y samples)
    case AV_PIX_FMT_YUV420P14: 			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_420 };			//planar YUV 4:2:0,21bpp, (1 Cr & Cb sample per 2x2 Y samples)
    case AV_PIX_FMT_YUV422P12: 			return { ColorFormat::G12X4_B12X4_R12X4_16, ColorSubsampling::RB_422 };	//planar YUV 4:2:2,24bpp, (1 Cr & Cb sample per 2x1 Y samples)
    case AV_PIX_FMT_YUV422P14: 			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_422 };			//planar YUV 4:2:2,28bpp, (1 Cr & Cb sample per 2x1 Y samples)
    case AV_PIX_FMT_YUV444P12: 			return { ColorFormat::G12X4_B12X4_R12X4_16, ColorSubsampling::RB_444 };	//planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples)
    case AV_PIX_FMT_YUV444P14: 			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_444 };			//planar YUV 4:4:4,42bpp, (1 Cr & Cb sample per 1x1 Y samples)
    case AV_PIX_FMT_GBRP12:    			return { ColorFormat::G12X4_B12X4_R12X4_16, ColorSubsampling::RB_444 };	//planar GBR 4:4:4 36bpp
    case AV_PIX_FMT_GBRP14:    			return { ColorFormat::G16_B16_R16, ColorSubsampling::RB_444 };			//planar GBR 4:4:4 42bpp
    //case AV_PIX_FMT_YUVJ411P:			return {}; /* DEPRECATED */ 	    									//planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples) full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV411P and setting color_range

    //case AV_PIX_FMT_BAYER_BGGR8:  	return {}; /* NOT SUPPORTED */ 											//bayer, BGBG..(odd line), GRGR..(even line), 8-bit samples
    //case AV_PIX_FMT_BAYER_RGGB8:  	return {}; /* NOT SUPPORTED */ 											//bayer, RGRG..(odd line), GBGB..(even line), 8-bit samples
    //case AV_PIX_FMT_BAYER_GBRG8:  	return {}; /* NOT SUPPORTED */ 											//bayer, GBGB..(odd line), RGRG..(even line), 8-bit samples
    //case AV_PIX_FMT_BAYER_GRBG8:  	return {}; /* NOT SUPPORTED */ 											//bayer, GRGR..(odd line), BGBG..(even line), 8-bit samples
    //case AV_PIX_FMT_BAYER_BGGR16:		return {}; /* NOT SUPPORTED */ 											//bayer, BGBG..(odd line), GRGR..(even line), 16-bit samples
    //case AV_PIX_FMT_BAYER_RGGB16:		return {}; /* NOT SUPPORTED */ 											//bayer, RGRG..(odd line), GBGB..(even line), 16-bit samples
    //case AV_PIX_FMT_BAYER_GBRG16:		return {}; /* NOT SUPPORTED */ 											//bayer, GBGB..(odd line), RGRG..(even line), 16-bit samples
    //case AV_PIX_FMT_BAYER_GRBG16:		return {}; /* NOT SUPPORTED */ 											//bayer, GRGR..(odd line), BGBG..(even line), 16-bit samples

    //case AV_PIX_FMT_XVMC:				return {}; /* NOT SUPPORTED */											//XVideo Motion Acceleration via common packet passing

    case AV_PIX_FMT_YUV440P10: 			return { ColorFormat::G10X6_B10X6_R10X6_16, ColorSubsampling::RB_440 };	//planar YUV 4:4:0,20bpp, (1 Cr & Cb sample per 1x2 Y samples)
    case AV_PIX_FMT_YUV440P12: 			return { ColorFormat::G12X4_B12X4_R12X4_16, ColorSubsampling::RB_440 };	//planar YUV 4:4:0,24bpp, (1 Cr & Cb sample per 1x2 Y samples)
    //case AV_PIX_FMT_AYUV64:   		return	{} /* NOT SUPPORTED TODO: Add support*/	 						//packed AYUV 4:4:4,64bpp (1 Cr & Cb sample per 1x1 Y & A samples)

    //case AV_PIX_FMT_VIDEOTOOLBOX:		return {}; /* NOT SUPPORTED */											//hardware decoding through Videotoolbox

    case AV_PIX_FMT_P010: 				return { ColorFormat::G10X6_B10X6R10X6_16, ColorSubsampling::RB_420 };	//like NV12, with 10bpp per component, data in the high bits, zeros in the low bits

    //case AV_PIX_FMT_GBRAP12:  		return	{} /* NOT SUPPORTED TODO: Add support*/							//planar GBR 4:4:4:4 48bpp
    //case AV_PIX_FMT_GBRAP10:  		return	{} /* NOT SUPPORTED TODO: Add support*/							//planar GBR 4:4:4:4 40bpp

    //case AV_PIX_FMT_MEDIACODEC: 		return {}; /* NOT SUPPORTED */											//hardware decoding through MediaCodec

    case AV_PIX_FMT_GRAY12:   			return { ColorFormat::Y12X4_16, ColorSubsampling::RB_444 };				//       Y        , 12bpp
    case AV_PIX_FMT_GRAY10:   			return { ColorFormat::Y10X6_16, ColorSubsampling::RB_444 };				//       Y        , 10bpp

    case AV_PIX_FMT_P016:  				return { ColorFormat::G16_B16R16, ColorSubsampling::RB_420 };			//like NV12, with 16bpp per component

    //case AV_PIX_FMT_D3D11:			return {}; /* NOT SUPPORTED */											//Hardware surfaces for Direct3D11.

    case AV_PIX_FMT_GRAY9:				return { ColorFormat::Y10X6_16, ColorSubsampling::RB_444 };				//       Y        , 9bpp

    case AV_PIX_FMT_GBRPF32:  			return {}; /* NOT SUPPORTED TODO: Add support*/							//IEEE-754 single precision planar GBR 4:4:4,     96bpp
    case AV_PIX_FMT_GBRAPF32: 			return {}; /* NOT SUPPORTED TODO: Add support*/							//IEEE-754 single precision planar GBRA 4:4:4:4, 128bpp

    //case AV_PIX_FMT_DRM_PRIME:		return {}; /* NOT SUPPORTED */											//DRM-managed buffers exposed through PRIME buffer sharing.

    //case AV_PIX_FMT_OPENCL:			return {}; /* NOT SUPPORTED */											//Hardware surfaces for OpenCL.

    case AV_PIX_FMT_GRAY14:   			return { ColorFormat::Y16, ColorSubsampling::RB_444 };					//       Y        , 14bpp

    case AV_PIX_FMT_GRAYF32:  			return { ColorFormat::Y32f, ColorSubsampling::RB_444 };					//IEEE-754 single precision Y, 32bpp

    //case AV_PIX_FMT_YUVA422P12: 		return {}; /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:2:2,24bpp, (1 Cr & Cb sample per 2x1 Y samples), 12b alpha
    //case AV_PIX_FMT_YUVA444P12: 		return {}; /* NOT SUPPORTED TODO: Add support*/							//planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples), 12b alpha

    case AV_PIX_FMT_NV24:      			return { ColorFormat::G8_B8R8, ColorSubsampling::RB_444 };				//planar YUV 4:4:4, 24bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
    case AV_PIX_FMT_NV42:     			return { ColorFormat::G8_R8B8, ColorSubsampling::RB_444 };				//as above, but U and V bytes are swapped

    //case AV_PIX_FMT_VULKAN:			return {}; /* NOT SUPPORTED */											//Vulkan hardware images.

    case AV_PIX_FMT_Y210:   			return { ColorFormat::G10X6B10X6G10X6R10X6_16, ColorSubsampling::RB_444 };//packed YUV 4:2:2 like YUYV422, 20bpp, data in the high bits


	default: return { ColorFormat::NONE, ColorSubsampling::NONE };
	}
}



constexpr AVMediaType toFFmpeg(MediaType type) {
	switch(type) {
	case MediaType::VIDEO: 		return AVMEDIA_TYPE_VIDEO;
	case MediaType::AUDIO: 		return AVMEDIA_TYPE_AUDIO;
	case MediaType::DATA: 		return AVMEDIA_TYPE_DATA;
	case MediaType::SUBTITLE: 	return AVMEDIA_TYPE_SUBTITLE;
	case MediaType::ATTACHMENT: return AVMEDIA_TYPE_ATTACHMENT;
	default: return AVMEDIA_TYPE_UNKNOWN;
	}
}

constexpr MediaType fromFFmpeg(AVMediaType type) {
	switch(type) {
	case AVMEDIA_TYPE_VIDEO: 		return MediaType::VIDEO;
	case AVMEDIA_TYPE_AUDIO: 		return MediaType::AUDIO;
	case AVMEDIA_TYPE_DATA: 		return MediaType::DATA;
	case AVMEDIA_TYPE_SUBTITLE: 	return MediaType::SUBTITLE;
	case AVMEDIA_TYPE_ATTACHMENT: 	return MediaType::ATTACHMENT;
	default: return MediaType::NONE;
	}
}



constexpr AVCodecID toFFmpeg(CodecId id) {
	return static_cast<AVCodecID>(id);
}

constexpr CodecId fromFFmpeg(AVCodecID id) {
	return static_cast<CodecId>(id);
}


}
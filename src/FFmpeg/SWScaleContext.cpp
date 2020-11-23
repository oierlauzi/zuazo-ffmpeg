#include "SWScaleContext.h"

extern "C" {
	#include <libswscale/swscale.h>
}


namespace Zuazo::FFmpeg {

SWScaleContext::SWScaleContext() 
	: m_handle(nullptr)
{
}

SWScaleContext::SWScaleContext(	Resolution srcRes,
								PixelFormat srcFmt,
								Resolution dstRes,
								PixelFormat dstFmt,
								int filter,
								SwsFilter* srcFilt,
								SwsFilter* dstFilt,
								const double* param )
	: m_handle(sws_getContext(
		srcRes.width, srcRes.height, static_cast<AVPixelFormat>(srcFmt),
		dstRes.width, dstRes.height, static_cast<AVPixelFormat>(dstFmt),
		filter, srcFilt, dstFilt, param
	))
{
}

SWScaleContext::SWScaleContext(SWScaleContext&& other)
	: m_handle(other.m_handle)
{
	other.m_handle = nullptr;
}

SWScaleContext::~SWScaleContext() {
	sws_freeContext(m_handle);
}



SWScaleContext& SWScaleContext::operator=(SWScaleContext&& other) {
	SWScaleContext(std::move(other)).swap(*this);
	return *this;
}



void SWScaleContext::swap(SWScaleContext& other) {
	std::swap(m_handle, other.m_handle);
}

void SWScaleContext::recreate(	Resolution srcRes,
								PixelFormat srcFmt,
								Resolution dstRes,
								PixelFormat dstFmt,
								int filter,
								SwsFilter* srcFilt,
								SwsFilter* dstFilt,
								const double* param )
{
	m_handle = sws_getCachedContext(
		m_handle,
		srcRes.width, srcRes.height, static_cast<AVPixelFormat>(srcFmt),
		dstRes.width, dstRes.height, static_cast<AVPixelFormat>(dstFmt),
		filter, srcFilt, dstFilt, param
	);
}


void SWScaleContext::scale(	std::byte const *const srcData[],
							const int srcStride[],
							int srcOff,
							int srcHeight,
							std::byte *const dstData[],
							const int dstStride[] ) const
{
	sws_scale(
		m_handle,
		reinterpret_cast<uint8_t const *const *>(srcData),
		reinterpret_cast<const int*>(srcStride),
		srcOff, srcHeight,
		reinterpret_cast<uint8_t *const *>(dstData),
		reinterpret_cast<const int*>(dstStride)
	);
}


bool SWScaleContext::isSupportedInput(FFmpeg::PixelFormat fmt) {
	return sws_isSupportedInput(static_cast<AVPixelFormat>(fmt));
}

bool SWScaleContext::isSupportedOutput(FFmpeg::PixelFormat fmt) {
	return sws_isSupportedOutput(static_cast<AVPixelFormat>(fmt));
}


}
#pragma once

#include <zuazo/FFmpeg/Enumerations.h>
#include <zuazo/FFmpeg/Frame.h>

struct SwsContext;
struct SwsFilter;

namespace Zuazo::FFmpeg {

class SWScaleContext {
public:
	using Handle = SwsContext*;
	using ConstHandle = const SwsContext*;

	SWScaleContext();
	SWScaleContext(	Resolution srcRes,
					PixelFormat srcFmt,
					Resolution dstRes,
					PixelFormat dstFmt,
					int filter = 0,
					SwsFilter* srcFilt = nullptr,
					SwsFilter* dstFilt = nullptr,
					const double* param = nullptr);
	SWScaleContext(const SWScaleContext& other) = delete;
	SWScaleContext(SWScaleContext&& other);
	~SWScaleContext();

	SWScaleContext& 					operator=(const SWScaleContext& other) = delete;
	SWScaleContext&						operator=(SWScaleContext&& other);

	operator Handle();
	operator ConstHandle() const;

	void								swap(SWScaleContext& other);

	void								recreate(	Resolution srcRes,
													PixelFormat srcFmt,
													Resolution dstRes,
													PixelFormat dstFmt,
													int filter = 0,
													SwsFilter* srcFilt = nullptr,
													SwsFilter* dstFilt = nullptr,
													const double* param = nullptr );

	void								scale(	std::byte const *const srcData[],
												const int srcStride[],
												int srcOff,
												int srcHeight,
												std::byte *const dstData[],
												const int dstStride[] ) const;

	static bool							isSupportedInput(FFmpeg::PixelFormat fmt);
	static bool							isSupportedOutput(FFmpeg::PixelFormat fmt);

private:
	Handle								m_handle;
	
	SWScaleContext&						get();
	const SWScaleContext&				get() const;
};

}
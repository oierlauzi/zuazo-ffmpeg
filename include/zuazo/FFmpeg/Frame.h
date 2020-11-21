#pragma once

#include "Enumerations.h"
#include "FrameSideData.h"

#include <zuazo/Resolution.h>
#include <zuazo/Utils/BufferView.h>

#include <cstddef>
#include <array>

struct AVFrame;

namespace Zuazo::FFmpeg {

class Frame {
public:
	using Handle = AVFrame*;
	using ConstHandle = const AVFrame*;

	Frame();
	Frame(ConstHandle hand);
	Frame(const Frame& other);
	Frame(Frame&& other);
	~Frame();

	Frame& 										operator=(const Frame& other);
	Frame&										operator=(Frame&& other);

	operator Handle();
	operator ConstHandle() const;

	void										swap(Frame& other);

	void								        unref();

	Utils::BufferView<std::byte*>				getData();
	Utils::BufferView<std::byte const* const>	getData() const;

	Utils::BufferView<int>						getLineSizes();
	Utils::BufferView<const int>				getLineSizes() const;

	void								        setResolution(Resolution res);
	Resolution							        getResolution() const;

	void										setSampleCount(int samples);
	int											getSampleCount() const;

	void								        setPixelFormat(PixelFormat fmt);
	PixelFormat							        getPixelFormat() const;

	void										setKeyFrame(bool key);
	bool										getKeyFrame() const;

	void								        setPictureType(PictureType type);
	PictureType							        getPictureType() const;

	void								        setPixelAspectRatio(AspectRatio par);
	AspectRatio							        getPixelAspectRatio() const;

	void										setPTS(int64_t pts);
	int64_t										getPTS() const;

	void										setPacketDTS(int64_t dts);
	int64_t										getPacketDTS() const;

	void										setCodecPictureNumber(int nb);
	int										    getCodecPictureNumber() const;

	void										setDisplayPictureNumber(int nb);
	int										    getDisplayPictureNumber() const;

	void										setQuality(int q);
	int										    getQuality() const;

	void										setRepeatPicture(int nb);
	int										    getRepeatPicture() const;

	void										setInterlaced(bool inter);
	bool										getInterlaced() const;

	void										setTopFieldFirst(bool top);
	bool										getTopFieldFirst() const;

	void										setPaletteHasChanged(bool pal);
	bool										getPaletteHasChanged() const;

	void										setSampleRate(int rate);
	int											getSampleRate() const;

	void										setChannelLayout(uint64_t layout);
	uint64_t									getChannelLayout() const;

	Utils::BufferView<FrameSideData>			getSideData();
	Utils::BufferView<const FrameSideData>		getSideData() const;

	void										setColorRange(ColorRange range);
	ColorRange              					getColorRange() const;

	void										setColorPrimaries(ColorPrimaries primaries);
	ColorPrimaries								getColorPrimaries() const;

	void										setColorTransferCharacteristic(ColorTransferCharacteristic trc);
	ColorTransferCharacteristic					getColorTransferCharacteristic() const;

	void										setColorSpace(ColorSpace space);
	ColorSpace					    			getColorSpace() const;

	void										setChromaLocation(ChromaLocation loc);
	ChromaLocation					   			getChromaLocation() const;

	void										setBestEffortTS(int64_t ts);
	int64_t										getBestEffortTS() const;

	void										setPacketPosition(int64_t pos);
	int64_t										getPacketPosition() const;

	void										setPacketDuration(int64_t dur);
	int64_t										getPacketDuration() const;

	void										setChannelCount(int nb);
	int											getChannelCount() const;

	void										setPacketSize(int sz);
	int											getPacketSize() const;

	void										setCropTop(size_t crop);
	size_t										getCropTop() const;

	void										setCropBottom(size_t crop);
	size_t										getCropBottom() const;

	void										setCropLeft(size_t crop);
	size_t										getCropLeft() const;

	void										setCropRight(size_t crop);
	size_t										getCropRight() const;

private:
	Handle										m_handle;
	
	AVFrame&									get();
	const AVFrame&								get() const;
};

}
#pragma once

#include "Enumerations.h"

#include <zuazo/Resolution.h>

#include <cstddef>

struct AVCodecParameters;

namespace Zuazo::FFmpeg {

class CodecParameters {
public:
	using Handle = AVCodecParameters*;
	using ConstHandle = const AVCodecParameters*;

	CodecParameters();
	CodecParameters(ConstHandle hand);
	CodecParameters(const CodecParameters& other);
	CodecParameters(CodecParameters&& other);
	~CodecParameters();

	CodecParameters& 					operator=(const CodecParameters& other);
	CodecParameters&					operator=(CodecParameters&& other);

	operator Handle();
	operator ConstHandle() const;

	void								swap(CodecParameters& other);


	void								setMediaType(MediaType type);
	MediaType							getMediaType() const;

	void								setCodecId(CodecID id);
	CodecID								getCodecId() const;

	void								setCodecTag(uint64_t tag);
	uint64_t							getCodecTag() const;

	void								setPixelFormat(PixelFormat fmt);
	PixelFormat							getPixelFormat() const;

	void								setBitrate(int64_t bps);
	int64_t								getBitrate() const;

	void								setBitsPerCodedSample(int bits);
	int							    	getBitsPerCodedSample() const;

    void								setBitsPerRawSample(int bits);
	int							    	getBitsPerRawSample() const;

    void								setProfile(int profile);
	int						            getProfile() const;

    void								setLevel(int level);
	int						            getLevel() const;

	void								setResolution(Resolution res);
	Resolution							getResolution() const;

    void								setPixelAspectRatio(AspectRatio par);
	AspectRatio							getPixelAspectRatio() const;

	void								setFieldOrder(FieldOrder field);
	FieldOrder							getFieldOrder() const;

	void								setColorRange(ColorRange range);
	ColorRange			                getColorRange() const;

    void								setColorPrimaries(ColorPrimaries primaries);
	ColorPrimaries						getColorPrimaries() const;

	void								setColorTransferCharacteristic(ColorTransferCharacteristic trc);
	ColorTransferCharacteristic			getColorTransferCharacteristic() const;

	void								setColorSpace(ColorSpace space);
	ColorSpace					    	getColorSpace() const;

	void								setChromaLocation(ChromaLocation loc);
	ChromaLocation					    getChromaLocation() const;

    void								setVideoDelay(int delay);
	int							    	getVideoDelay() const;

    void								setChannelCount(int cnt);
	int							    	getChannelCount() const;
    
    void								setChannelLayout(uint64_t mask);
	uint64_t						    getChannelLayout() const;

    void								setSampleRate(int rate);
	int						            getSampleRate() const;

    void								setBlockAlign(int align);
	int						            getBlockAlign() const;

    void								setFrameSize(int size);
	int						            getFrameSize() const;

    void								setInitialPadding(int padding);
	int						            getInitialPadding() const;

    void								setTrailingPadding(int padding);
	int						            getTrailingPadding() const;

    void								setSeekPreroll(int seekPrerol);
	int						            getSeekPreroll() const;

private:
	Handle								m_handle;

    AVCodecParameters&                  get();
    const AVCodecParameters&            get() const;
	
};

}
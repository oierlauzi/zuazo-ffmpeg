#include <zuazo/FFmpeg/CodecParameters.h>

extern "C" {
	#include <libavcodec/codec_par.h>
}

#include <cassert>

namespace Zuazo::FFmpeg {

CodecParameters::CodecParameters() 
	: m_handle(avcodec_parameters_alloc())
{
}

CodecParameters::CodecParameters(ConstHandle hand) 
	: CodecParameters()
{
	avcodec_parameters_copy(m_handle, hand);
}


CodecParameters::CodecParameters(const CodecParameters& other)
	: CodecParameters(other.m_handle)
{
}

CodecParameters::CodecParameters(CodecParameters&& other) 
	: m_handle(other.m_handle)
{
	other.m_handle = nullptr;
}

CodecParameters::~CodecParameters() {
	//nullptr can be passed
	avcodec_parameters_free(&m_handle);
}



CodecParameters& CodecParameters::operator=(const CodecParameters& other) {
	CodecParameters(other).swap(*this);
	return *this;
}

CodecParameters& CodecParameters::operator=(CodecParameters&& other) {
	CodecParameters(std::move(other)).swap(*this);
	return *this;
}



CodecParameters::operator Handle() {
	return m_handle;
}

CodecParameters::operator ConstHandle() const {
	return m_handle;
}



void CodecParameters::swap(CodecParameters& other) {
	std::swap(m_handle, other.m_handle);
}



void CodecParameters::setMediaType(MediaType type) {
	get().codec_type = static_cast<AVMediaType>(type);
}

MediaType CodecParameters::getMediaType() const {
	return static_cast<MediaType>(get().codec_type);
}


void CodecParameters::setCodecId(CodecID id) {
	get().codec_id = static_cast<AVCodecID>(id);
}

CodecID CodecParameters::getCodecId() const {
	return static_cast<CodecID>(get().codec_id);
}


void CodecParameters::setCodecTag(uint64_t tag) {
	get().codec_tag = tag;
}

uint64_t CodecParameters::getCodecTag() const {
	return get().codec_tag;
}


void CodecParameters::setPixelFormat(PixelFormat fmt) {
	get().format = static_cast<AVPixelFormat>(fmt);
}

PixelFormat CodecParameters::getPixelFormat() const {
	return static_cast<PixelFormat>(get().format);
}


void CodecParameters::setBitrate(int64_t bps) {
	get().bit_rate = bps;
}

int64_t CodecParameters::getBitrate() const {
	return get().bit_rate;
}


void CodecParameters::setBitsPerCodedSample(int bits) {
	get().bits_per_coded_sample = bits;
}

int CodecParameters::getBitsPerCodedSample() const {
	return get().bits_per_coded_sample;
}


void CodecParameters::setBitsPerRawSample(int bits) {
	get().bits_per_raw_sample = bits;
}

int CodecParameters::getBitsPerRawSample() const {
	return get().bits_per_raw_sample;
}


void CodecParameters::setProfile(int profile) {
	get().profile = profile;
}

int CodecParameters::getProfile() const {
	return get().profile;
}


void CodecParameters::setLevel(int level) {
	get().level = level;
}

int CodecParameters::getLevel() const {
	return get().level;
}


void CodecParameters::setResolution(Resolution res) {
	get().width = res.width;
	get().height = res.height;
}

Resolution CodecParameters::getResolution() const {
	return Resolution(
		get().width, 
		get().height
	);
}


void CodecParameters::setPixelAspectRatio(AspectRatio par) {
	get().sample_aspect_ratio.num = par.getNumerator();
	get().sample_aspect_ratio.den = par.getDenominator();
}

AspectRatio CodecParameters::getPixelAspectRatio() const {
	return AspectRatio(
		get().sample_aspect_ratio.num, 
		get().sample_aspect_ratio.den
	);
}


void CodecParameters::setFieldOrder(FieldOrder field) {
	get().field_order = static_cast<AVFieldOrder>(field);
}

FieldOrder CodecParameters::getFieldOrder() const {
	return static_cast<FieldOrder>(get().field_order);
}


void CodecParameters::setColorRange(ColorRange range) {
	get().color_range = static_cast<AVColorRange>(range);
}

ColorRange CodecParameters::getColorRange() const {
	return static_cast<ColorRange>(get().color_range);
}


void CodecParameters::setColorPrimaries(ColorPrimaries primaries) {
	get().color_primaries = static_cast<AVColorPrimaries>(primaries);
}

ColorPrimaries CodecParameters::getColorPrimaries() const {
	return static_cast<ColorPrimaries>(get().color_primaries);
}


void CodecParameters::setColorTransferCharacteristic(ColorTransferCharacteristic trc) {
	get().color_trc = static_cast<AVColorTransferCharacteristic>(trc);
}

ColorTransferCharacteristic CodecParameters::getColorTransferCharacteristic() const {
	return static_cast<ColorTransferCharacteristic>(get().color_trc);
}


void CodecParameters::setColorSpace(ColorSpace space) {
	get().color_space = static_cast<AVColorSpace>(space);
}

ColorSpace CodecParameters::getColorSpace() const {
	return static_cast<ColorSpace>(get().color_space);
}


void CodecParameters::setChromaLocation(ChromaLocation loc) {
	get().chroma_location = static_cast<AVChromaLocation>(loc);
}

ChromaLocation CodecParameters::getChromaLocation() const {
	return static_cast<ChromaLocation>(get().chroma_location);
}


void CodecParameters::setVideoDelay(int delay) {
	get().video_delay = delay;
}

int CodecParameters::getVideoDelay() const {
	return get().video_delay;
}


void CodecParameters::setChannelCount(int cnt) {
	get().channels = cnt;
}

int CodecParameters::getChannelCount() const {
	return get().channels;
}


void CodecParameters::setChannelLayout(uint64_t mask) {
	get().channel_layout = mask;
}

uint64_t CodecParameters::getChannelLayout() const {
	return get().channel_layout;
}


void CodecParameters::setSampleRate(int rate) {
	get().sample_rate = rate;
}

int CodecParameters::getSampleRate() const {
	return get().sample_rate;
}


void CodecParameters::setBlockAlign(int align) {
	get().block_align = align;
}

int CodecParameters::getBlockAlign() const {
	return get().block_align;
}


void CodecParameters::setFrameSize(int size) {
	get().frame_size = size;
}

int CodecParameters::getFrameSize() const {
	return get().frame_size;
}


void CodecParameters::setInitialPadding(int padding) {
	get().initial_padding = padding;
}

int CodecParameters::getInitialPadding() const {
	return get().initial_padding;
}


void CodecParameters::setTrailingPadding(int padding) {
	get().trailing_padding = padding;
}

int CodecParameters::getTrailingPadding() const {
	return get().trailing_padding;
}


void CodecParameters::setSeekPreroll(int seekPrerol) {
	get().seek_preroll = seekPrerol;
}

int CodecParameters::getSeekPreroll() const {
	return get().seek_preroll;
}



AVCodecParameters& CodecParameters::get() {
	assert(m_handle);
	return *m_handle;
}

const AVCodecParameters& CodecParameters::get() const {
	assert(m_handle);
	return *m_handle;
}

}
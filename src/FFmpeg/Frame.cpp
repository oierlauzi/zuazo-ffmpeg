#include <zuazo/FFmpeg/Frame.h>

extern "C" {
	#include <libavutil/frame.h>
}

namespace Zuazo::FFmpeg {

Frame::Frame() 
	: m_handle(av_frame_alloc())
{
}

Frame::Frame(ConstHandle hand) 
	: m_handle(av_frame_clone(hand))
{
}


Frame::Frame(const Frame& other)
	: Frame(other.m_handle)
{
}

Frame::Frame(Frame&& other) 
	: m_handle(other.m_handle)
{
	other.m_handle = nullptr;
}

Frame::~Frame() {
	//nullptr can be passed
	av_frame_free(&m_handle);
}



Frame& Frame::operator=(const Frame& other) {
	Frame(other).swap(*this);
	return *this;
}

Frame& Frame::operator=(Frame&& other) {
	Frame(std::move(other)).swap(*this);
	return *this;
}



Frame::operator Handle() {
	return m_handle;
}

Frame::operator ConstHandle() const {
	return m_handle;
}



void Frame::swap(Frame& other) {
	std::swap(m_handle, other.m_handle);
}



void Frame::unref() {
	av_frame_unref(&get());
}



Utils::BufferView<std::byte*> Frame::getData() {
	assert(m_handle);
	return Utils::BufferView<std::byte*>(reinterpret_cast<std::byte**>(get().data), AV_NUM_DATA_POINTERS);
}

Utils::BufferView<std::byte const* const> Frame::getData() const {
	assert(m_handle);
	return Utils::BufferView<std::byte const* const>(reinterpret_cast<std::byte const* const*>(get().data), AV_NUM_DATA_POINTERS);
}


Utils::BufferView<int> Frame::getLineSizes() {
	assert(m_handle);
	return Utils::BufferView<int>(get().linesize);
}

Utils::BufferView<const int> Frame::getLineSizes() const {
	assert(m_handle);
	return Utils::BufferView<const int>(get().linesize);
}


void Frame::setResolution(Resolution res) {
	get().width = res.width;
	get().height = res.height;
}

Resolution Frame::getResolution() const {
	return Resolution(
		get().width,
		get().height
	);
}


void Frame::setSampleCount(int samples) {
	get().nb_samples = samples;
}

int Frame::getSampleCount() const {
	return get().nb_samples;
}


void Frame::setPixelFormat(PixelFormat fmt) {
	get().format = static_cast<int>(fmt);
}

PixelFormat Frame::getPixelFormat() const {
	return static_cast<PixelFormat>(get().format);
}


void Frame::setKeyFrame(bool key) {
	get().key_frame = key ? 1 : 0;
}

bool Frame::getKeyFrame() const {
	return get().key_frame;
}


void Frame::setPictureType(PictureType type) {
	get().pict_type = static_cast<AVPictureType>(type);
}

PictureType Frame::getPictureType() const {
	return static_cast<PictureType>(get().pict_type);
}


void Frame::setPixelAspectRatio(AspectRatio par) {
	get().sample_aspect_ratio.num = par.getNumerator();
	get().sample_aspect_ratio.den = par.getDenominator();
}

AspectRatio Frame::getPixelAspectRatio() const {
	return AspectRatio(
		get().sample_aspect_ratio.num,
		get().sample_aspect_ratio.den
	);
}


void Frame::setPTS(int64_t pts) {
	get().pts = pts;
}

int64_t Frame::getPTS() const {
	return get().pts;
}


void Frame::setPacketDTS(int64_t dts) {
	get().pkt_dts = dts;
}

int64_t Frame::getPacketDTS() const {
	return get().pkt_dts;
}


void Frame::setCodecPictureNumber(int nb) {
	get().coded_picture_number = nb;
}

int Frame::getCodecPictureNumber() const {
	return get().coded_picture_number;
}


void Frame::setDisplayPictureNumber(int nb) {
	get().display_picture_number = nb;
}

int Frame::getDisplayPictureNumber() const {
	return get().display_picture_number;
}


void Frame::setQuality(int q) {
	get().quality = q;
}

int Frame::getQuality() const {
	return get().quality;
}


void Frame::setRepeatPicture(int nb) {
	get().repeat_pict = nb;
}

int Frame::getRepeatPicture() const {
	return get().repeat_pict;
}


void Frame::setInterlaced(bool inter) {
	get().interlaced_frame = inter;
}

bool Frame::getInterlaced() const {
	return get().interlaced_frame;
}


void Frame::setTopFieldFirst(bool top) {
	get().top_field_first = top;
}

bool Frame::getTopFieldFirst() const {
	return get().top_field_first;
}


void Frame::setPaletteHasChanged(bool pal) {
	get().palette_has_changed = pal;
}

bool Frame::getPaletteHasChanged() const {
	return get().palette_has_changed;
}


void Frame::setSampleRate(int rate) {
	get().sample_rate = rate;
}

int Frame::getSampleRate() const {
	return get().sample_rate;
}


void Frame::setChannelLayout(uint64_t layout) {
	get().channel_layout = layout;
}

uint64_t Frame::getChannelLayout() const {
	return get().channel_layout;
}


Utils::BufferView<FrameSideData> Frame::getSideData() {
	static_assert(sizeof(AVFrameSideData*) == sizeof(FrameSideData), "Pointer size and side data size must match");
	return Utils::BufferView<FrameSideData>(
		reinterpret_cast<FrameSideData*>(get().side_data),
		static_cast<size_t>(get().nb_side_data)
	);
}

Utils::BufferView<const FrameSideData> Frame::getSideData() const {
	static_assert(sizeof(const AVFrameSideData*) == sizeof(FrameSideData), "Pointer size and side data size must match");
	return Utils::BufferView<const FrameSideData>(
		reinterpret_cast<FrameSideData*>(get().side_data),
		static_cast<size_t>(get().nb_side_data)
	);
}


void Frame::setColorRange(ColorRange range) {
	get().color_range = static_cast<AVColorRange>(range);
}

ColorRange Frame::getColorRange() const {
	return static_cast<ColorRange>(get().color_range);
}


void Frame::setColorPrimaries(ColorPrimaries primaries) {
	get().color_primaries = static_cast<AVColorPrimaries>(primaries);
}

ColorPrimaries Frame::getColorPrimaries() const {
	return static_cast<ColorPrimaries>(get().color_primaries);
}


void Frame::setColorTransferCharacteristic(ColorTransferCharacteristic trc) {
	get().color_trc = static_cast<AVColorTransferCharacteristic>(trc);
}

ColorTransferCharacteristic Frame::getColorTransferCharacteristic() const {
	return static_cast<ColorTransferCharacteristic>(get().color_trc);
}


void Frame::setColorSpace(ColorSpace space) {
	get().colorspace = static_cast<AVColorSpace>(space);
}

ColorSpace Frame::getColorSpace() const {
	return static_cast<ColorSpace>(get().colorspace);
}


void Frame::setChromaLocation(ChromaLocation loc) {
	get().chroma_location = static_cast<AVChromaLocation>(loc);
}

ChromaLocation Frame::getChromaLocation() const {
	return static_cast<ChromaLocation>(get().chroma_location);
}


void Frame::setBestEffortTS(int64_t ts) {
	get().best_effort_timestamp = ts;
}

int64_t Frame::getBestEffortTS() const {
	return get().best_effort_timestamp;
}


void Frame::setPacketPosition(int64_t pos) {
	get().pkt_pos = pos;
}

int64_t Frame::getPacketPosition() const {
	return get().pkt_pos;
}


void Frame::setPacketDuration(int64_t dur) {
	get().pkt_duration = dur;
}

int64_t Frame::getPacketDuration() const {
	return get().pkt_duration;
}


void Frame::Frame::setChannelCount(int nb) {
	get().channels = nb;
}

int Frame::Frame::getChannelCount() const {
	return get().channels;
}


void Frame::setPacketSize(int sz) {
	get().pkt_size = sz;
}

int Frame::getPacketSize() const {
	return get().pkt_size;
}


void Frame::setCropTop(size_t crop) {
	get().crop_top = crop;
}

size_t Frame::getCropTop() const {
	return get().crop_top;
}


void Frame::setCropBottom(size_t crop) {
	get().crop_bottom = crop;
}

size_t Frame::getCropBottom() const {
	return get().crop_bottom;
}


void Frame::setCropLeft(size_t crop) {
	get().crop_left = crop;
}

size_t Frame::getCropLeft() const {
	return get().crop_left;
}


void Frame::setCropRight(size_t crop) {
	get().crop_right = crop;
}

size_t Frame::getCropRight() const {
	return get().crop_right;
}



AVFrame& Frame::get() {
	assert(m_handle);
	return *m_handle;
}

const AVFrame& Frame::get() const {
	assert(m_handle);
	return *m_handle;
}

}
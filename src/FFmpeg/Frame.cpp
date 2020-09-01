#include "Frame.h"

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



void Frame::setPTS(int64_t pts) {
	assert(m_handle);
	m_handle->pts = pts;
}

int64_t Frame::getPTS() const {
	assert(m_handle);
	return m_handle->pts;
}



void Frame::setResolution(Resolution res) {
	assert(m_handle);
	m_handle->width = res.width;
	m_handle->height = res.height;
}

Resolution Frame::getResolution() const {
	assert(m_handle);
	return Resolution(m_handle->width, m_handle->height);
}



Utils::BufferView<std::byte*> Frame::getData() {
	assert(m_handle);
	return Utils::BufferView<std::byte*>(reinterpret_cast<std::byte**>(m_handle->data), AV_NUM_DATA_POINTERS);
}

Utils::BufferView<std::byte const* const> Frame::getData() const {
	assert(m_handle);
	return Utils::BufferView<std::byte const* const>(reinterpret_cast<std::byte const* const*>(m_handle->data), AV_NUM_DATA_POINTERS);
}



Utils::BufferView<int> Frame::getLineSizes() {
	assert(m_handle);
	return Utils::BufferView<int>(m_handle->linesize);
}

Utils::BufferView<const int> Frame::getLineSizes() const {
	assert(m_handle);
	return Utils::BufferView<int>(m_handle->linesize);
}


}
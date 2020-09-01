#include <zuazo/FFmpeg/Packet.h>

extern "C" {
	#include <libavcodec/packet.h>
}

namespace Zuazo::FFmpeg {

Packet::Packet() 
	: m_handle(av_packet_alloc())
{
}

Packet::Packet(size_t size) 
	: Packet() 
{
	av_new_packet(m_handle, size);
}

Packet::Packet(ConstHandle hand) 
	: m_handle(av_packet_clone(hand))
{
}


Packet::Packet(const Packet& other)
	: Packet(other.m_handle)
{
}

Packet::Packet(Packet&& other) 
	: m_handle(other.m_handle)
{
	other.m_handle = nullptr;
}

Packet::~Packet() {
	//nullptr can be passed
	av_packet_free(&m_handle);
}



Packet& Packet::operator=(const Packet& other) {
	Packet(other).swap(*this);
	return *this;
}

Packet& Packet::operator=(Packet&& other) {
	Packet(std::move(other)).swap(*this);
	return *this;
}



Packet::operator Handle() {
	return m_handle;
}

Packet::operator ConstHandle() const {
	return m_handle;
}



void Packet::swap(Packet& other) {
	std::swap(m_handle, other.m_handle);
}



void Packet::setPTS(int64_t pts) {
	assert(m_handle);
	m_handle->pts = pts;
}

int64_t Packet::getPTS() const {
	assert(m_handle);
	return m_handle->pts;
}



void Packet::setDTS(int64_t dts) {
	assert(m_handle);
	m_handle->dts = dts;
}

int64_t Packet::getDTS() const {
	assert(m_handle);
	return m_handle->dts;
}



void Packet::setDuration(int64_t duration) {
	assert(m_handle);
	m_handle->duration = duration; 
}

int64_t Packet::getDuration() const {
	assert(m_handle);
	return m_handle->duration; 
}



void Packet::setStreamIndex(int idx) {
	assert(m_handle);
	m_handle->stream_index = idx; 
}

int Packet::getStreamIndex() const {
	assert(m_handle);
	return m_handle->stream_index; 
}



Utils::BufferView<std::byte> Packet::getData() {
	assert(m_handle);
	return Utils::BufferView<std::byte>(reinterpret_cast<std::byte*>(m_handle->data), m_handle->size);
}

Utils::BufferView<const std::byte>Packet::getData() const {
	assert(m_handle);
	return Utils::BufferView<const std::byte>(reinterpret_cast<std::byte*>(m_handle->data), m_handle->size);
}



void Packet::shrink(size_t size) {
	assert(m_handle);
	av_shrink_packet(m_handle, size);
}

void Packet::grow(size_t size) {
	assert(m_handle);
	av_grow_packet(m_handle, size);
}

}
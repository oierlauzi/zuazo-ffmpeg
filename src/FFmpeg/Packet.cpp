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



void Packet::unref() {
	av_packet_unref(&get());
}



void Packet::setPTS(int64_t pts) {
	get().pts = pts;
}

int64_t Packet::getPTS() const {
	return get().pts;
}


void Packet::setDTS(int64_t dts) {
	get().dts = dts;
}

int64_t Packet::getDTS() const {
	return get().dts;
}


void Packet::setDuration(int64_t duration) {
	get().duration = duration; 
}

int64_t Packet::getDuration() const {
	return get().duration; 
}


void Packet::setPosition(int64_t pos) {
	get().pos = pos; 
}

int64_t Packet::getPosition() const {
	return get().pos;
}


void Packet::setStreamIndex(int idx) {
	get().stream_index = idx; 
}

int Packet::getStreamIndex() const {
	return get().stream_index; 
}


Utils::BufferView<std::byte> Packet::getData() {
	return Utils::BufferView<std::byte>(reinterpret_cast<std::byte*>(get().data), get().size);
}

Utils::BufferView<const std::byte>Packet::getData() const {
	return Utils::BufferView<const std::byte>(reinterpret_cast<std::byte*>(get().data), get().size);
}


void Packet::shrink(size_t size) {
	av_shrink_packet(&get(), size);
}

void Packet::grow(size_t size) {
	av_grow_packet(&get(), size);
}



AVPacket& Packet::get() {
	assert(m_handle);
	return *m_handle;
}

const AVPacket& Packet::get() const {
	assert(m_handle);
	return *m_handle;
}

}
#include "CodecContext.h"

namespace Zuazo::FFmpeg {

CodecContext::CodecContext(Codec codec) 
	: m_handle(avcodec_alloc_context3(codec))
{
}

CodecContext::CodecContext(CodecContext&& other)
	: m_handle(other.m_handle)
{
	other.m_handle = nullptr;
}

CodecContext::~CodecContext() {
	avcodec_free_context(&m_handle);
}



CodecContext& CodecContext::operator=(CodecContext&& other) {
	CodecContext(std::move(other)).swap(*this);
	return *this;
}



void CodecContext::swap(CodecContext& other) {
	std::swap(m_handle, other.m_handle);
}



void CodecContext::open(const Codec& codec) {
	avcodec_open2(m_handle, codec, nullptr);
}



void CodecContext::setParameters(const AVCodecParameters& parameters) {
	avcodec_parameters_to_context(m_handle, &parameters);
}

void CodecContext::getParameters(AVCodecParameters& parameters) const {
	avcodec_parameters_from_context(&parameters, m_handle);
}

AVCodecParameters CodecContext::getParameters() const {
	AVCodecParameters result;
	getParameters(result);
	return result;
}



int CodecContext::sendPacket(const Packet& packet) {
	return avcodec_send_packet(m_handle, packet);
}

int CodecContext::readPacket(Packet& packet) {
	return avcodec_receive_packet(m_handle, packet);
}

int CodecContext::sendFrame(const Frame& frame) {
	return avcodec_send_frame(m_handle, frame);
}

int CodecContext::readFrame(Frame& frame) {
	return avcodec_receive_frame(m_handle, frame);
}


}
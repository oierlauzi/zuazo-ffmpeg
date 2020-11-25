#include "CodecContext.h"

extern "C" {
	#include <libavcodec/avcodec.h>
}

#include <cassert>

namespace Zuazo::FFmpeg {

CodecContext::CodecContext() 
	: m_handle(nullptr)
{
}

CodecContext::CodecContext(const AVCodec *codec) 
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


CodecContext::operator Handle() {
	return m_handle;
}

CodecContext::operator ConstHandle() const {
	return m_handle;
}



int CodecContext::open(const AVCodec *codec) {
	return avcodec_open2(&get(), codec, nullptr);
}


void CodecContext::setOpaque(void* userData) {
	get().opaque = userData;
}

void* CodecContext::getOpaque() const {
	return get().opaque;
}


int CodecContext::setParameters(const CodecParameters& parameters) {
	return avcodec_parameters_to_context(&get(), parameters);
}

int CodecContext::getParameters(CodecParameters& parameters) const {
	return avcodec_parameters_from_context(parameters, &get());
}

CodecParameters CodecContext::getParameters() const {
	CodecParameters result;
	getParameters(result);
	return result;
}


void CodecContext::setPixelFormatNegotiationCallback(PixelFormatNegotiationCallback cbk) {
	get().get_format = reinterpret_cast<AVPixelFormat (*)(AVCodecContext *s, const AVPixelFormat *fmt)>(cbk);
}

CodecContext::PixelFormatNegotiationCallback CodecContext::getPixelFormatNegotiationCallback() const {
	return reinterpret_cast<PixelFormatNegotiationCallback>(get().get_format);
}



void CodecContext::setThreadCount(int cnt) {
	get().thread_count = cnt;
}

int CodecContext::getThreadCount() const {
	return get().thread_count;
}


void CodecContext::setThreadType(ThreadType type) {
	get().thread_type = static_cast<int>(type);
}

ThreadType CodecContext::getThreadType() const {
	return static_cast<ThreadType>(get().thread_type);
}



int CodecContext::sendPacket(const Packet& packet) {
	return avcodec_send_packet(&get(), packet);
}

int CodecContext::readPacket(Packet& packet) {
	return avcodec_receive_packet(&get(), packet);
}

int CodecContext::sendFrame(const Frame& frame) {
	return avcodec_send_frame(&get(), frame);
}

int CodecContext::readFrame(Frame& frame) {
	return avcodec_receive_frame(&get(), frame);
}



void CodecContext::flush() {
	avcodec_flush_buffers(&get());
}



AVCodecContext& CodecContext::get() {
	assert(m_handle);
	return *m_handle;
}

const AVCodecContext& CodecContext::get() const {
	assert(m_handle);
	return *m_handle;
}


}
#include "InputFormatContext.h"

namespace Zuazo::FFmpeg {

InputFormatContext::InputFormatContext(const char* url) 
	: m_handle(nullptr)
{
	if(avformat_open_input(&m_handle, url, NULL, NULL) != 0) {
		return; //Opening the input
	}

	if(avformat_find_stream_info(m_handle, NULL) < 0)
		return; //Error getting stream info

}

InputFormatContext::InputFormatContext(InputFormatContext&& other)
	: m_handle(other.m_handle)
{
	other.m_handle = nullptr;
}

InputFormatContext::~InputFormatContext() {
	avformat_close_input(&m_handle);
}



InputFormatContext& InputFormatContext::operator=(InputFormatContext&& other) {
	InputFormatContext(std::move(other)).swap(*this);
	return *this;
}



void InputFormatContext::swap(InputFormatContext& other) {
	std::swap(m_handle, other.m_handle);
}



int InputFormatContext::getStreamCount() const {
	return get().nb_streams;
}

int InputFormatContext::findBestStream(MediaType type) const {
	assert(m_handle);
	return av_find_best_stream(
		m_handle,						//AVInputFormatContext handle
		static_cast<AVMediaType>(type),	//AVMediaType
		-1,								//Wanted stream number
		-1,								//Related stream
		nullptr,						//Decoder list
		0								//Flags
	);
}



int InputFormatContext::play() {
	return av_read_play(&get());
}

int InputFormatContext::pause() {
	return av_read_pause(&get());
}

int InputFormatContext::seek(int stream, int64_t timestamp) {
	return av_seek_frame(
		&get(),					//Handle
		stream,					//Stream index
		timestamp,				//Time stamp
		0						//Flags
	);
}

int InputFormatContext::seek(int64_t timestamp) {
	return seek(-1, timestamp);
}

int InputFormatContext::seekAny(int stream, int64_t timestamp) {
	return av_seek_frame(
		&get(),					//Handle
		stream,					//Stream index
		timestamp,				//Time stamp
		AVSEEK_FLAG_ANY			//Flags
	);
}

int InputFormatContext::seekAny(int64_t timestamp) {
	return seekAny(-1, timestamp);
}

int InputFormatContext::readPacket(Packet& pkt) {
	return av_read_frame(
		&get(),					//AVInputFormatContext handle
		static_cast<Packet::Handle>(pkt) //Packet handle
	);
}

int InputFormatContext::flush() {
	return avformat_flush(&get());
}



AVFormatContext& InputFormatContext::get() {
	assert(m_handle);
	return *m_handle;
}

const AVFormatContext& InputFormatContext::get() const {
	assert(m_handle);
	return *m_handle;
}

}
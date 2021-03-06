#include "InputFormatContext.h"

#include <zuazo/Exception.h>

extern "C" {
	#include <libavformat/avformat.h>
}

namespace Zuazo::FFmpeg {

InputFormatContext::InputFormatContext(const char* url) 
	: m_handle(nullptr)
{
	if(avformat_open_input(&m_handle, url, NULL, NULL) != 0) {
		//Opening the input
		throw Exception("Unable to open the input for file: " + std::string(url));
	}

	if(avformat_find_stream_info(m_handle, NULL) < 0) {
		//Error getting stream info
		throw Exception("Unable to find stream info for the file: " + std::string(url));
	}

	//At this point it should have been successful
	assert(m_handle);
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



InputFormatContext::Streams InputFormatContext::getStreams() const {
	static_assert(sizeof(StreamParameters::Handle) == sizeof(StreamParameters), "In order to reinterpret cast, size must match");
	return Streams(reinterpret_cast<const StreamParameters*>(get().streams), get().nb_streams);
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

Duration InputFormatContext::getDuration() const {
	return Duration(get().duration);
}


int InputFormatContext::play() {
	return av_read_play(&get());
}

int InputFormatContext::pause() {
	return av_read_pause(&get());
}

int InputFormatContext::seek(int stream, int64_t timestamp, SeekFlags flags) {
	return av_seek_frame(
		&get(),					//Handle
		stream,					//Stream index
		timestamp,				//Time stamp
		static_cast<int>(flags)	//Flags
	);
}

int InputFormatContext::seek(Duration timestamp, SeekFlags flags) {	
	return seek(-1, timestamp.count(), flags);
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
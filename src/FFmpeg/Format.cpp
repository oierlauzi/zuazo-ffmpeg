#include "Format.h"

#include "FFmpegConversions.h"

namespace Zuazo::FFmpeg {

FormatContext::FormatContext(const char* url) 
	: m_handle(nullptr)
{
	if(avformat_open_input(&m_handle, url, NULL, NULL) != 0) {
		return; //Opening the input
	}

	if(avformat_find_stream_info(m_handle, NULL) < 0)
		return; //Error getting stream info

}

FormatContext::FormatContext(FormatContext&& other)
	: m_handle(other.m_handle)
{
	other.m_handle = nullptr;
}

FormatContext::~FormatContext() {
	avformat_close_input(&m_handle);
}



FormatContext& FormatContext::operator=(FormatContext&& other) {
	FormatContext(std::move(other)).swap(*this);
	return *this;
}



void FormatContext::swap(FormatContext& other) {
	std::swap(m_handle, other.m_handle);
}



int FormatContext::writeHeader() {
	return avformat_write_header(
		m_handle,
		nullptr
	);
}



int FormatContext::findBestStream(MediaType type) const {
	return av_find_best_stream(
		m_handle,				//AVFormatContext handle
		toFFmpeg(type),			//AVMediaType
		-1,						//Wanted stream number
		-1,						//Related stream
		nullptr,				//Decoder list
		0						//Flags
	);
}



int FormatContext::play() {
	return av_read_play(m_handle);
}

int FormatContext::pause() {
	return av_read_pause(m_handle);
}

int FormatContext::seek(int stream, int64_t timestamp) {
	return av_seek_frame(
		m_handle,				//Hanle
		stream,					//Stream index
		timestamp,				//Time stamp
		0						//Flags
	);
}

int FormatContext::seekAny(int stream, int64_t timestamp) {
	return av_seek_frame(
		m_handle,				//Hanle
		stream,					//Stream index
		timestamp,				//Time stamp
		AVSEEK_FLAG_ANY			//Flags
	);
}

int FormatContext::readPacket(Packet& pkt) {
	return av_read_frame(
		m_handle,				//AVFormatContext handle
		static_cast<Packet::Handle>(pkt) //Packet handle
	);
}

int FormatContext::writePacket(Packet& pkt) {
	return av_interleaved_write_frame(
		m_handle,				//AVFormatContext handle
		static_cast<Packet::Handle>(pkt) //Packet handle
	);
}

int FormatContext::flush() {
	return avformat_flush(m_handle);
}

}
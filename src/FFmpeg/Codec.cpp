#include "Codec.h"

#include "FFmpegConversions.h"

extern "C" {
	#include <libavcodec/codec.h>
}


namespace Zuazo::FFmpeg {

Codec::Codec(Handle handle)
	: m_handle(handle)
{
}


Codec::operator Handle() {
	return m_handle;
}

Codec::operator ConstHandle() const {
	return m_handle;
}



Encoder::Encoder(CodecId codecId)
	: Codec(avcodec_find_encoder(toFFmpeg(codecId)))
{
}


Encoder::Encoder(const char* name)
	: Codec(avcodec_find_encoder_by_name(name))
{
}



Decoder::Decoder(CodecId codecId)
	: Codec(avcodec_find_decoder(toFFmpeg(codecId)))
{
}

Decoder::Decoder(const char* name)
	: Codec(avcodec_find_decoder_by_name(name))
{
}


}
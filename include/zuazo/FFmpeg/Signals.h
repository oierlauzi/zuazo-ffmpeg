#pragma once

#include "Packet.h"
#include "Frame.h"

#include <zuazo/Signal/NamingConventions.h>

#include <memory>
#include <string_view>

namespace Zuazo::FFmpeg {

using PacketStream = std::shared_ptr<const FFmpeg::Packet>;
using Video = std::shared_ptr<const FFmpeg::Frame>;

}

namespace Zuazo::Signal {

template<>
constexpr std::string_view makeInputName<FFmpeg::PacketStream>() { return "packetIn"; }

template<>
constexpr std::string_view makeOutputName<FFmpeg::PacketStream>() { return "packetOut"; }


template<>
constexpr std::string_view makeInputName<FFmpeg::Video>() { return "videoIn"; }

template<>
constexpr std::string_view makeOutputName<FFmpeg::Video>() { return "videoOut"; }

}
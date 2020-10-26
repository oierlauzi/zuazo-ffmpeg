#pragma once

#include "Packet.h"
#include "Frame.h"

#include <zuazo/Signal/NamingConventions.h>

#include <memory>
#include <string_view>

namespace Zuazo::FFmpeg {

using PacketStream = std::shared_ptr<const FFmpeg::Packet>;
using FrameStream = std::shared_ptr<const FFmpeg::Frame>;

}

namespace Zuazo::Signal {

template<>
constexpr std::string_view makeInputName<FFmpeg::PacketStream>() noexcept { return "packetIn"; }

template<>
constexpr std::string_view makeOutputName<FFmpeg::PacketStream>() noexcept { return "packetOut"; }


template<>
constexpr std::string_view makeInputName<FFmpeg::FrameStream>() noexcept { return "frameIn"; }

template<>
constexpr std::string_view makeOutputName<FFmpeg::FrameStream>() noexcept { return "frameOut"; }

}
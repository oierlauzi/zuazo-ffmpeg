#pragma once

#include <chrono>

namespace Zuazo::FFmpeg {

using Duration = std::chrono::duration<int64_t, std::micro>;

}
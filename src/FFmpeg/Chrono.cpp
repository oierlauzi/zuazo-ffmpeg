#include <zuazo/FFmpeg/Chrono.h>

extern "C" {
	#include <libavutil/avutil.h>
}

namespace Zuazo::FFmpeg {

static constexpr auto TIME_BASE = AV_TIME_BASE_Q;
static_assert(Duration::period::num == TIME_BASE.num, "Timestamp representation period numerator does not match");
static_assert(Duration::period::den == TIME_BASE.den, "Timestamp representation period denominator does not match");
static_assert(std::is_same<Duration::rep, int64_t>::value, "Timestamp representation type does not match");


}
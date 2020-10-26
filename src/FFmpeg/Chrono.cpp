#include <zuazo/FFmpeg/Chrono.h>

extern "C" {
	#include <libavutil/avutil.h>
}

namespace Zuazo::FFmpeg {

static_assert(Duration::period::num == 1, "Timestamp representation period numerator does not match");
static_assert(Duration::period::den == AV_TIME_BASE, "Timestamp representation period denominator does not match");
static_assert(std::is_same<Duration::rep, int64_t>::value, "Timestamp representation type does not match");


}
#include <zuazo/FFmpeg/StreamParameters.h>

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/codec_par.h>
}

#include <cassert>

namespace Zuazo::FFmpeg {

StreamParameters::operator Handle() {
	return m_handle;
}

StreamParameters::operator ConstHandle() const {
	return m_handle;
}



void StreamParameters::setIndex(int i) {
	get().index = i;
}

int StreamParameters::getIndex() const {
	return get().index;
}


void StreamParameters::setID(int id) {
	get().id = id;
}

int StreamParameters::getID() const {
	return get().id;
}


void StreamParameters::setTimeBase(Math::Rational<int> tb) {
	get().time_base.num = tb.getNumerator();
	get().time_base.den = tb.getDenominator();
}

Math::Rational<int> StreamParameters::getTimeBase() const {
	return Math::Rational<int>(
		get().time_base.num,
		get().time_base.den
	);
}


void StreamParameters::setStartTime(int64_t st) {
	get().start_time = st;
}

int64_t StreamParameters::getStartTime() const {
	return get().start_time;
}	
	

void StreamParameters::setDuration(int64_t dur) {
	get().duration = dur;
}

int64_t StreamParameters::getDuration() const {
	return get().duration;
}							

				
void StreamParameters::setFrameCount(int64_t cnt) {
	get().nb_frames = cnt;
}

int64_t StreamParameters::getFrameCount() const {
	return get().nb_frames;
}	


void StreamParameters::setDisposition(int disp) {
	get().disposition = disp;
}

int StreamParameters::getDisposition() const {
	return get().disposition;
}


void StreamParameters::setDiscard(Discard disc) {
	get().discard = static_cast<AVDiscard>(disc);
}

Discard StreamParameters::getDiscard() const {
	return static_cast<Discard>(get().discard);
}


void StreamParameters::setSampleAspectRatio(AspectRatio aspect) {
	get().sample_aspect_ratio.num = aspect.getNumerator();
	get().sample_aspect_ratio.den = aspect.getDenominator();
}

AspectRatio StreamParameters::getSampleAspectRatio() const {
	return AspectRatio(
		get().sample_aspect_ratio.num,
		get().sample_aspect_ratio.den
	);
}


void StreamParameters::setAverageFrameRate(Math::Rational<int> rate) {
	get().avg_frame_rate.num = rate.getNumerator();
	get().avg_frame_rate.den = rate.getDenominator();
}

Math::Rational<int> StreamParameters::getAverageFrameRate() const {
	return Math::Rational<int>(
		get().avg_frame_rate.num,
		get().avg_frame_rate.den
	);
}


Packet StreamParameters::getAttachedPicture() const {
	return Packet(&get().attached_pic);
}


void StreamParameters::setEventFlags(int flags) {
	get().event_flags = flags;
}

int StreamParameters::getEventFlags() const {
	return get().event_flags;
}


void StreamParameters::setRealFrameRate(Math::Rational<int> rate) {
	get().r_frame_rate.num = rate.getNumerator();
	get().r_frame_rate.den = rate.getDenominator();
}

Math::Rational<int> StreamParameters::getRealFrameRate() const {
	return Math::Rational<int>(
		get().r_frame_rate.num,
		get().r_frame_rate.den
	);
}


void StreamParameters::setCodecParameters(const CodecParameters& codecPar) {
	assert(static_cast<CodecParameters::ConstHandle>(codecPar));
	assert(get().codecpar);

	avcodec_parameters_copy(get().codecpar, codecPar);
}

const CodecParameters& StreamParameters::getCodecParameters() const {
	static_assert(sizeof(CodecParameters::Handle) == sizeof(CodecParameters), "In order to reinterpret cast, size must match");
	return reinterpret_cast<const CodecParameters&>(get().codecpar);
}



AVStream& StreamParameters::get() {
	assert(m_handle);
	return *m_handle;
}

const AVStream& StreamParameters::get() const {
	assert(m_handle);
	return *m_handle;
}

}
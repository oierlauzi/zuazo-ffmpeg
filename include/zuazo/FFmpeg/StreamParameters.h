#pragma once

#include "Packet.h"
#include "CodecParameters.h"
#include "Enumerations.h"

#include <zuazo/Resolution.h>
#include <zuazo/Math/Rational.h>

#include <cstddef>

struct AVStream;

namespace Zuazo::FFmpeg {

class StreamParameters {
public:
	using Handle = AVStream*;
	using ConstHandle = const AVStream*;

	StreamParameters(const StreamParameters& other) = delete;
	StreamParameters(StreamParameters&& other) = delete;
	~StreamParameters() = delete;

	StreamParameters& 					operator=(const StreamParameters& other) = delete;
	StreamParameters&					operator=(StreamParameters&& other) = delete;

	operator Handle();
	operator ConstHandle() const;

	void								setIndex(int i);
	int							    	getIndex() const;

	void								setID(int id);
	int							    	getID() const;

	void								setTimeBase(Math::Rational<int> tb);
	Math::Rational<int>					getTimeBase() const;

	void								setStartTime(int64_t st);
	int64_t								getStartTime() const;			

	void								setDuration(int64_t dur);
	int64_t								getDuration() const;								
					
	void								setFrameCount(int64_t cnt);
	int64_t								getFrameCount() const;	

	void								setDisposition(int disp);
	int									getDisposition() const;

	void								setDiscard(Discard disc);
	Discard								getDiscard() const;

	void								setSampleAspectRatio(AspectRatio aspect);
	AspectRatio							getSampleAspectRatio() const;

	void								setAverageFrameRate(Math::Rational<int> rate);
	Math::Rational<int>					getAverageFrameRate() const;

	Packet								getAttachedPicture() const;

	void								setEventFlags(int flags);
	int									getEventFlags() const;

	void								setRealFrameRate(Math::Rational<int> rate);
	Math::Rational<int>					getRealFrameRate() const;

	void								setCodecParameters(const CodecParameters& codecPar);
	const CodecParameters&				getCodecParameters() const;

private:
	Handle								m_handle;

    AVStream&							get();
    const AVStream&						get() const;
	
};

}
#include <zuazo/FFmpeg/FrameSideData.h>

extern "C" {
	#include <libavutil/frame.h>
}

#include <cassert>

namespace Zuazo::FFmpeg {

FrameSideData::operator Handle() {
	return m_handle;
}

FrameSideData::operator ConstHandle() const {
	return m_handle;
}


void FrameSideData::setType(FrameSideDataType type) {
	get().type = static_cast<AVFrameSideDataType>(type);
}

FrameSideDataType FrameSideData::getType() const {
	return static_cast<FrameSideDataType>(get().type);
}


Utils::BufferView<std::byte> FrameSideData::getData() {
	return Utils::BufferView<std::byte>(
		reinterpret_cast<std::byte*>(get().data), 
		static_cast<size_t>(get().size)
	);
}

Utils::BufferView<const std::byte> FrameSideData::getData() const {
	return Utils::BufferView<const std::byte>(
		reinterpret_cast<std::byte*>(get().data), 
		static_cast<size_t>(get().size)
	);
}



AVFrameSideData& FrameSideData::get() {
	assert(m_handle);
	return *m_handle;
}

const AVFrameSideData& FrameSideData::get() const {
	assert(m_handle);
	return *m_handle;
}


}
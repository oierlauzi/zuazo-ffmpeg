#pragma once

#include <zuazo/Resolution.h>
#include <zuazo/Utils/BufferView.h>

#include <cstddef>
#include <array>

struct AVFrame;

namespace Zuazo::FFmpeg {

class Frame {
public:
	using Handle = AVFrame*;
	using ConstHandle = const AVFrame*;

	Frame();
	Frame(ConstHandle hand);
	Frame(const Frame& other);
	Frame(Frame&& other);
	~Frame();

	Frame& 										operator=(const Frame& other);
	Frame&										operator=(Frame&& other);

	operator Handle();
	operator ConstHandle() const;

	void										swap(Frame& other);

	void										setPTS(int64_t pts);
	int64_t										getPTS() const;

	void										setResolution(Resolution res);
	Resolution									getResolution() const;

	Utils::BufferView<std::byte*>				getData();
	Utils::BufferView<std::byte const* const>	getData() const;

	Utils::BufferView<int>						getLineSizes();
	Utils::BufferView<const int>				getLineSizes() const;
	

private:
	Handle										m_handle;
	
};

}
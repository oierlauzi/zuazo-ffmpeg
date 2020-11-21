#pragma once

#include "Enumerations.h"

#include <zuazo/Utils/BufferView.h>

#include <cstddef>

struct AVFrameSideData;

namespace Zuazo::FFmpeg {

class FrameSideData {
public:
	using Handle = AVFrameSideData*;
	using ConstHandle = const AVFrameSideData*;

	FrameSideData(const FrameSideData& other) = delete;
	FrameSideData(FrameSideData&& other) = delete;
	~FrameSideData() = delete;

	FrameSideData& 								operator=(const FrameSideData& other) = delete;
	FrameSideData&								operator=(FrameSideData&& other) = delete;

	operator Handle();
	operator ConstHandle() const;

	void										setType(FrameSideDataType type);
	FrameSideDataType							getType() const;

	Utils::BufferView<std::byte>				getData();
	Utils::BufferView<const std::byte>			getData() const;

private:
	Handle										m_handle;
	
	AVFrameSideData&							get();
	const AVFrameSideData&						get() const;
};

}
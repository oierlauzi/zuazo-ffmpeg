#include "MediaType.h"

#include <zuazo/Macros.h>

namespace Zuazo {

constexpr std::string_view toString(FFmpeg::MediaType type) {
	switch(type){

	ZUAZO_ENUM2STR_CASE( FFmpeg::MediaType, VIDEO )
	ZUAZO_ENUM2STR_CASE( FFmpeg::MediaType, AUDIO )
	ZUAZO_ENUM2STR_CASE( FFmpeg::MediaType, DATA )
	ZUAZO_ENUM2STR_CASE( FFmpeg::MediaType, SUBTITLE )
	ZUAZO_ENUM2STR_CASE( FFmpeg::MediaType, ATTACHMENT )

	default: return "";
	}
}

inline std::ostream& operator<<(std::ostream& os, FFmpeg::MediaType type) {
	return os << toString(type);
}

}
#include "util/uix.hpp"
#include <algorithm>
#include <string_view>

template<ztu::usize Size>
inline bool safe_strcpy(const std::string_view& str, uint8_t (&buffer)[Size]) {
	
	if (str.size() >= Size)
		return false;

	std::copy(str.begin(), str.end(), buffer);
	buffer[str.size()] = '\0';
	
	return true;
};

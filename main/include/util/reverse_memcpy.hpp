#include "util/uix.hpp"

inline void* reverse_memcpy(void* dst, const void* src, ztu::usize n) {

	auto dst_bytes = static_cast<ztu::u8*>(dst);
	auto src_bytes = static_cast<const ztu::u8*>(src);

	for (ztu::usize i{}; i != n; ++i) {
		dst_bytes[n - i - 1] = src_bytes[i];
	}

	return dst;
}

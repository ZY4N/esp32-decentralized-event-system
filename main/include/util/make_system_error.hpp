#pragma once

#include <system_error>

inline std::error_code make_system_error(int code) {
	using errc_t = std::underlying_type_t<std::errc>;
	const auto errc = static_cast<std::errc>(static_cast<errc_t>(code));
	return std::make_error_code(errc);
}

#pragma once

#include <system_error>

namespace hash_engine_error {
	enum class codes {
		ok,
		use_before_initialization,
		wrong_hash_size,
		hashing_failed
	};

	struct category : std::error_category {
		const char* name() const noexcept override {
			return "hashing";
		}
		std::string message(int ev) const override {
			switch (static_cast<codes>(ev)) {
				using enum codes;
			case ok:
				return "Operation completed successfully.";
			case use_before_initialization:
				return "Attempted to use the hashing engine before initialization.";
			case wrong_hash_size:
				return "Given hash buffer size is incorrect.";
			case hashing_failed:
				return "Internal hashing operationg failed.";
			default:
				return "Unknown error.";
			}
		}
	};
}

inline hash_engine_error::category& hash_engine_category() {
	static hash_engine_error::category cat;
	return cat;
}

namespace hash_engine_error {
	inline std::error_code make_error_code(codes e) {
		return { static_cast<int>(e), hash_engine_category() };
	}
}

template <>
struct std::is_error_code_enum<hash_engine_error::codes> : public std::true_type {};

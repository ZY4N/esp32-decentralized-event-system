#pragma once

#include <system_error>

namespace signature_engine_error {
	enum class codes {
		ok,
		wrong_private_key_size,
		wrong_public_key_size,
		wrong_signature_size,
		setting_seed_failed,
		setting_key_pair_failed,
		use_before_initialization,
		initializing_elliptic_curve_failed,
		initializing_random_seed_failed,
		hashing_failed,
		encryption_failed,
		decryption_failed,
		signature_contruction_failed,
		signature_extraction_failed
	};

	struct category : std::error_category {
		const char* name() const noexcept override {
			return "signing";
		}
		std::string message(int ev) const override {
			switch (static_cast<codes>(ev)) {
				using enum codes;
			case ok:
				return "Operation completed successfully.";
			case wrong_private_key_size:
				return "Private key size is incorrect.";
			case wrong_public_key_size:
				return "Public key size is incorrect.";
			case wrong_signature_size:
				return "Signature size is incorrect.";
			case setting_seed_failed:
				return "Failed to set cryptographic seed.";
			case setting_key_pair_failed:
				return "Failed to set and initialize key pair.";
			case use_before_initialization:
				return "Attempted to use the signing engine before initialization.";
			case initializing_elliptic_curve_failed:
				return "Initializing the elliptic curve group failed.";
			case hashing_failed:
				return "Hashing operation failed.";
			case encryption_failed:
				return "Encryption operation failed.";
			case decryption_failed:
				return "Decryption operation failed.";
			case signature_contruction_failed:
				return "Failed to construct the signature.";
			case signature_extraction_failed:
				return "Failed to extract the signature.";
			default:
				return "Unknown error.";
			}
		}
	};
}

inline signature_engine_error::category& signature_engine_category() {
	static signature_engine_error::category cat;
	return cat;
}

namespace signature_engine_error {
	inline std::error_code make_error_code(codes e) {
		return { static_cast<int>(e), signature_engine_category() };
	}
}

template <>
struct std::is_error_code_enum<signature_engine_error::codes> : public std::true_type {};

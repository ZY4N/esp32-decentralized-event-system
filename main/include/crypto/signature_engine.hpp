#pragma once

#include "util/uix.hpp"
#include <system_error>
#include <span>

#include "crypto/hash_engine.hpp"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"


class signature_engine {
public:

	static constexpr ztu::usize private_key_size = 32;
	static constexpr ztu::usize public_key_size = 1 + 64;
	static constexpr ztu::usize component_size = 32;
	static constexpr ztu::usize signature_size = 2 * component_size;

public:
	signature_engine() = default;

	signature_engine(signature_engine&& other);

	signature_engine& operator=(signature_engine&& other);

	signature_engine(const signature_engine& other) = delete;

	signature_engine& operator=(const signature_engine& other) = delete;

	~signature_engine();

	
	[[nodiscard]] std::error_code init(std::span<const ztu::u8> private_key);

	[[nodiscard]] std::error_code sign(std::span<const ztu::u8> message, std::span<ztu::u8> signature);

	[[nodiscard]] std::error_code set_signature_key(std::span<const ztu::u8> private_key);

	[[nodiscard]] std::error_code set_verification_key(std::span<const ztu::u8> public_key);

	[[nodiscard]] std::error_code verify(
		std::span<const ztu::u8> message,
		std::span<const ztu::u8> signature,
		bool& correct
	);

private:
	hash_engine m_hash_engine;
	mbedtls_entropy_context m_entropy_ctx;
    mbedtls_ctr_drbg_context m_ctr_drbg_ctx;
    mbedtls_ecdsa_context m_ecdsa_ctx;

	static constexpr ztu::u8
		state_uninitialized = 0b0,
		state_hash_engine_initialized = (1 << 0),
		state_mbedtls_initialized = (1 << 1),
		state_curve_initialized = (1 << 2),
		state_signature_key_set = (1 << 3),
		state_verification_key_set = (1 << 4),
		state_backend_initialized = (
			state_hash_engine_initialized |
			state_mbedtls_initialized |
			state_curve_initialized
		),
		state_signature_initialized = (
			state_backend_initialized |
			state_signature_key_set
		),
		state_verification_initialized = (
			state_backend_initialized |
			state_verification_key_set
		);


	ztu::u8 m_state{ state_uninitialized };
};

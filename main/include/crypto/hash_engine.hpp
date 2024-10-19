#pragma once

#include "util/uix.hpp"
#include <system_error>
#include <array>
#include <span>

#include "mbedtls/sha256.h"

class hash_engine {
public:

	static constexpr ztu::usize hash_size = 32;

public:
	hash_engine() = default;

	hash_engine(hash_engine&& other);
	hash_engine& operator=(hash_engine&& other);

	hash_engine(const hash_engine& other) = delete;
	hash_engine& operator=(const hash_engine& other) = delete;

	[[nodiscard]] std::error_code init();

	[[nodiscard]] std::error_code hash(
		std::span<const uint8_t> message,
		std::span<uint8_t> digest
	);

	~hash_engine();

private:
	mbedtls_sha256_context m_sha256_ctx;
	
	bool m_initilized{ false }; // necessary as access to ctx is private
};

#include "crypto/hash_engine.hpp"
#include "crypto/hash_engine_error.hpp"

#include "util/feed_the_beast.hpp"

hash_engine::hash_engine(hash_engine&& other) :
	m_sha256_ctx{ other.m_sha256_ctx },
	m_initilized{ other.m_initilized }
{
	other.m_initilized = false;
}

hash_engine& hash_engine::operator=(hash_engine&& other) {
	if (&other != this) {
		this->~hash_engine();
		m_sha256_ctx = other.m_sha256_ctx;
		m_initilized = other.m_initilized;
		other.m_initilized = false;
	}
	return *this;
}

std::error_code hash_engine::init() {
	feed_the_beast();
	mbedtls_sha256_init(&m_sha256_ctx);
	m_initilized = true;
	return {};
}

std::error_code hash_engine::hash(
	std::span<const uint8_t> message,
	std::span<uint8_t> digest
) {
	using enum hash_engine_error::codes;
	using hash_engine_error::make_error_code;

	if (not m_initilized) {
		return use_before_initialization;
	}

	if (digest.size() != hash_size) {
		return wrong_hash_size;
	}

	feed_the_beast();

	if (mbedtls_sha256_starts(&m_sha256_ctx, 0) != 0) // 0 = SHA-256, 1 = SHA-224
		return hashing_failed;
	
	if (mbedtls_sha256_update(&m_sha256_ctx, message.data(), message.size()) != 0)
		return hashing_failed;

	if (mbedtls_sha256_finish(&m_sha256_ctx, digest.data()) != 0)
		return hashing_failed;

	return {};
}

hash_engine::~hash_engine() {
	if (m_initilized) {
		feed_the_beast();
		mbedtls_sha256_free(&m_sha256_ctx);
	}
}

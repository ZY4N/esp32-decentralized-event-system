#include "crypto/signature_engine.hpp"
#include "crypto/signature_engine_error.hpp"

#include "mbedtls/error.h"
#include "util/feed_the_beast.hpp"


signature_engine::signature_engine(signature_engine&& other) :
	m_hash_engine{ std::move(other.m_hash_engine) },
	m_ctr_drbg_ctx{ other.m_ctr_drbg_ctx },
	m_ecdsa_ctx{ other.m_ecdsa_ctx },
	m_state{ other.m_state }
{
	other.m_state = state_uninitialized;
}

signature_engine& signature_engine::operator=(signature_engine&& other) {
	if (&other != this) {
		this->~signature_engine();
		this->m_hash_engine = std::move(other.m_hash_engine);
		this->m_ctr_drbg_ctx = other.m_ctr_drbg_ctx;
		this->m_ecdsa_ctx = other.m_ecdsa_ctx;
		this->m_state = other.m_state;
		other.m_state = state_uninitialized;
	}
	return *this;
}

std::error_code signature_engine::init(std::span<const ztu::u8> private_key) {

	using enum signature_engine_error::codes;
	using signature_engine_error::make_error_code;

	if (auto error = m_hash_engine.init(); error) {
		return error;
	}

	m_state |= state_hash_engine_initialized;

	feed_the_beast();

	mbedtls_ecdsa_init(&m_ecdsa_ctx);
	// MBEDTLS_ECP_DP_SECP256R1 MBEDTLS_ECP_DP_BP256R1
	if (mbedtls_ecp_group_load(&m_ecdsa_ctx.MBEDTLS_PRIVATE(grp), MBEDTLS_ECP_DP_SECP256R1) != 0)
    	return make_error_code(initializing_elliptic_curve_failed);

	m_state |= state_curve_initialized;

	feed_the_beast();

	mbedtls_entropy_init(&m_entropy_ctx);
	mbedtls_ctr_drbg_init(&m_ctr_drbg_ctx);

	feed_the_beast();

	if (mbedtls_ctr_drbg_seed(&m_ctr_drbg_ctx, mbedtls_entropy_func, &m_entropy_ctx, nullptr, 0) != 0)
		return make_error_code(initializing_random_seed_failed);

	m_state |= state_mbedtls_initialized;

	if (auto error = set_signature_key(private_key); error) {
		return error;
	}

	return {};
}

std::error_code signature_engine::set_signature_key(std::span<const ztu::u8> private_key) {
	
	using enum signature_engine_error::codes;
	using signature_engine_error::make_error_code;

	feed_the_beast();
	
	if (private_key.size() != private_key_size)
		return make_error_code(wrong_private_key_size);

	if (mbedtls_mpi_read_binary(&m_ecdsa_ctx.MBEDTLS_PRIVATE(d), private_key.data(), private_key.size()) != 0)
		return make_error_code(setting_key_pair_failed);

	m_state |= state_signature_key_set;

	return {};
}

std::error_code signature_engine::set_verification_key(std::span<const ztu::u8> public_key) {
	
	using enum signature_engine_error::codes;
	using signature_engine_error::make_error_code;
	
	if (public_key.size() != public_key_size)
		return make_error_code(wrong_public_key_size);

	feed_the_beast();

	if (mbedtls_ecp_point_read_binary(
		&m_ecdsa_ctx.MBEDTLS_PRIVATE(grp),
		&m_ecdsa_ctx.MBEDTLS_PRIVATE(Q),
		public_key.data(), public_key.size()) != 0
	) {
		return make_error_code(setting_key_pair_failed);
	}

	m_state |= state_verification_key_set;

	return {};
}

std::error_code signature_engine::sign(
	std::span<const ztu::u8> message,
	std::span<ztu::u8> signature
) {

	using enum signature_engine_error::codes;
	using signature_engine_error::make_error_code;

	if ((m_state & state_signature_initialized) != state_signature_initialized)
		return make_error_code(use_before_initialization);

	if (signature.size() != signature_size)
		return make_error_code(wrong_signature_size);

	std::array<ztu::u8, hash_engine::hash_size> hash;
	if (auto error = m_hash_engine.hash(message, hash); error) {
		return error;
	}

	feed_the_beast();

	mbedtls_mpi r, s;
	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&s);

	auto error = std::error_code{};

	feed_the_beast();

	if ((mbedtls_ecdsa_sign(
		&m_ecdsa_ctx.MBEDTLS_PRIVATE(grp),
		&r, &s,
		&m_ecdsa_ctx.MBEDTLS_PRIVATE(d),
		hash.data(), hash.size(),
		mbedtls_ctr_drbg_random, &m_ctr_drbg_ctx)) != 0
	) {
		error = make_error_code(encryption_failed);
		goto on_error;
	}

	feed_the_beast();

	if (mbedtls_mpi_write_binary(&r, signature.data(), component_size) != 0) {
		error = make_error_code(signature_contruction_failed);
		goto on_error;
	}

	if (mbedtls_mpi_write_binary(&s, signature.data() + component_size, component_size) != 0) {
		error = make_error_code(signature_contruction_failed);
		goto on_error;
	}

on_error:
	mbedtls_mpi_free(&r);
	mbedtls_mpi_free(&s);

	return error;
}

std::error_code signature_engine::verify(
	std::span<const ztu::u8> message,
	std::span<const ztu::u8> signature,
	bool& correct
) {

	using enum signature_engine_error::codes;
	using signature_engine_error::make_error_code;

	if ((m_state & state_verification_initialized) != state_verification_initialized)
		return make_error_code(use_before_initialization);

	if (signature.size() != signature_size)
		return make_error_code(wrong_signature_size);

	std::array<ztu::u8, hash_engine::hash_size> hash;
	if (auto error = m_hash_engine.hash(message, hash); error) {
		return error;
	}

	feed_the_beast();

	mbedtls_mpi r, s;
	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&s);

	auto error = std::error_code{};

	if (mbedtls_mpi_read_binary(&r, signature.data(), component_size) != 0) {
		error = make_error_code(signature_extraction_failed);
		goto on_error;
	}

	if (mbedtls_mpi_read_binary(&s, signature.data() + component_size, component_size) != 0) {
		error = make_error_code(signature_extraction_failed);
		goto on_error;
	}

	feed_the_beast();

	switch (mbedtls_ecdsa_verify(
		&m_ecdsa_ctx.MBEDTLS_PRIVATE(grp),
		hash.data(), hash.size(),
		&m_ecdsa_ctx.MBEDTLS_PRIVATE(Q),
		&r, &s
	)) {
		case 0:
			correct = true;
			break;
		case MBEDTLS_ERR_ECP_VERIFY_FAILED:
			correct = false;
			break;
		default:
			error = make_error_code(decryption_failed);
			break;
	}

on_error:
	mbedtls_mpi_free(&r);
	mbedtls_mpi_free(&s);

	return error;
}

signature_engine::~signature_engine() {
	if (m_state & state_backend_initialized) {
		feed_the_beast();
		mbedtls_entropy_free(&m_entropy_ctx);
		mbedtls_ctr_drbg_free(&m_ctr_drbg_ctx);
		mbedtls_ecdsa_free(&m_ecdsa_ctx);
	}
}

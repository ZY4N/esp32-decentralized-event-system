#pragma once

#include "lwip/sockets.h"
#include "util/uix.hpp"
#include <system_error>
#include <optional>
#include "lwip/sockets.h"
#include "util/make_system_error.hpp"


namespace socket_address {

using any_ip_t = sockaddr_storage;
using ipv4_t = sockaddr_in;
using ipv6_t = sockaddr_in6;
using ip_ptr_t = sockaddr*;
using const_ip_ptr_t = const sockaddr*;
using ip_size_t = socklen_t;

[[nodiscard]] inline ip_ptr_t to_generic_ptr(any_ip_t& ip);
[[nodiscard]] inline const_ip_ptr_t to_generic_ptr(const any_ip_t& ip);

[[nodiscard]] inline ip_ptr_t to_generic_ptr(ipv4_t& ipv4);
[[nodiscard]] inline const_ip_ptr_t to_generic_ptr(const ipv4_t& ipv4);

[[nodiscard]] inline ip_ptr_t to_generic_ptr(ipv6_t& ipv6);
[[nodiscard]] inline const_ip_ptr_t to_generic_ptr(const ipv6_t& ipv6);


[[nodiscard]] inline bool is_ipv4(const any_ip_t& ip);
[[nodiscard]] inline bool is_ipv6(const any_ip_t& ip);

[[nodiscard]] inline ipv4_t& to_ipv4(any_ip_t& ip);
[[nodiscard]] inline const ipv4_t& to_ipv4(const any_ip_t& ip);

[[nodiscard]] inline ipv6_t& to_ipv6(any_ip_t& ip);
[[nodiscard]] inline const ipv6_t& to_ipv6(const any_ip_t& ip);


[[nodiscard]] inline ipv4_t any_ipv4(const ztu::u16 port);
[[nodiscard]] inline ipv4_t broadcast_ipv4(const ztu::u16 port);

inline void init_ipv4(const ztu::u16 port, ipv4_t& address, in_addr_t host = INADDR_ANY);
[[nodiscard]] inline std::error_code init_ipv4(const char* host, const ztu::u16 port, ipv4_t& address);


[[nodiscard]] inline ipv6_t any_ipv6(const ztu::u16 port);

inline void init_ipv6(const ztu::u16 port, ipv6_t& address);
[[nodiscard]] inline std::error_code init_ipv6(const char* host, const ztu::u16 port, ipv6_t& address);

[[nodiscard]] inline std::optional<std::array<char, INET_ADDRSTRLEN>> ipv4_to_str(const ipv4_t& ipv4);
[[nodiscard]] inline std::optional<std::array<char, INET6_ADDRSTRLEN>> ipv6_to_str(const ipv6_t& ipv6);
[[nodiscard]] inline std::optional<std::array<char, INET6_ADDRSTRLEN>> any_ip_to_str(const any_ip_t& ip);

}

namespace socket_address {

ip_ptr_t to_generic_ptr(any_ip_t& ip) {
	return reinterpret_cast<ip_ptr_t>(&ip);
}

const_ip_ptr_t to_generic_ptr(const any_ip_t& ip) {
	return reinterpret_cast<const_ip_ptr_t>(&ip);
}

ip_ptr_t to_generic_ptr(ipv4_t& ipv4) {
	return reinterpret_cast<ip_ptr_t>(&ipv4);
}

const_ip_ptr_t to_generic_ptr(const ipv4_t& ipv4) {
	return reinterpret_cast<const_ip_ptr_t>(&ipv4);
}

ip_ptr_t to_generic_ptr(ipv6_t& ipv6) {
	return reinterpret_cast<ip_ptr_t>(&ipv6);
}

const_ip_ptr_t to_generic_ptr(const ipv6_t& ipv6) {
	return reinterpret_cast<const_ip_ptr_t>(&ipv6);
}


[[nodiscard]] inline bool is_ipv4(const any_ip_t& ip) {
	return ip.ss_family == AF_INET;
}

[[nodiscard]] inline bool is_ipv6(const any_ip_t& ip) {
	return ip.ss_family == AF_INET6;
}

ipv4_t& to_ipv4(any_ip_t& ip) {
	return *reinterpret_cast<ipv4_t*>(&ip);
}

const ipv4_t& to_ipv4(const any_ip_t& ip) {
	return *reinterpret_cast<const ipv4_t*>(&ip);
}

ipv6_t& to_ipv6(any_ip_t& ip) {
	return *reinterpret_cast<ipv6_t*>(&ip);
}

const ipv6_t& to_ipv6(const any_ip_t& ip) {
	return *reinterpret_cast<const ipv6_t*>(&ip);
}


void init_ipv4(const ztu::u16 port, ipv4_t& ipv4, in_addr_t host) {
	ipv4 = {
		.sin_len = sizeof(ipv4_t),
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr = {
			.s_addr = htonl(host)
		},
		.sin_zero = {}
	};
}

ipv4_t any_ipv4(const ztu::u16 port) {
	ipv4_t ipv4;
	init_ipv4(port, ipv4, INADDR_ANY);
	return ipv4;
}

ipv4_t broadcast_ipv4(const ztu::u16 port) {
	ipv4_t ipv4;
	init_ipv4(port, ipv4, INADDR_BROADCAST);
	return ipv4;
}

std::error_code init_ipv4(const char* host, const ztu::u16 port, ipv4_t& ipv4) {
	init_ipv4(port, ipv4);

	if (auto ret = inet_pton(AF_INET, host, &ipv4.sin_addr); ret < 1) {
		return make_system_error(ret == 0 ? EINVAL : errno);
	}

	return {};
}


void init_ipv6(const ztu::u16 port, ipv6_t& ipv6) {
	ipv6 = {
		.sin6_len = sizeof(ipv6_t),
		.sin6_family = AF_INET6,
		.sin6_port = htons(port),
		.sin6_flowinfo = 0,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_scope_id = 0
	};
}

ipv6_t any_ipv6(const ztu::u16 port) {
	ipv6_t ipv6;
	init_ipv6(port, ipv6);
	return ipv6;
}

std::error_code init_ipv6(const char* host, const ztu::u16 port, ipv6_t& ipv6) {
	init_ipv6(port, ipv6);

	if (auto ret = inet_pton(AF_INET6, host, &ipv6.sin6_addr); ret < 1) {
		return make_system_error(ret == 0 ? EINVAL : errno);
	}

	return {};
}



std::optional<std::array<char, INET_ADDRSTRLEN>> ipv4_to_str(const ipv4_t& ipv4) {
	std::array<char, INET_ADDRSTRLEN> buffer;
	if (inet_ntop(AF_INET, &ipv4.sin_addr, buffer.data(), buffer.size()) != nullptr) {
		return buffer;
	} else {
		return std::nullopt;
	}
}

std::optional<std::array<char, INET6_ADDRSTRLEN>> ipv6_to_str(const ipv6_t& ipv6) {
	std::array<char, INET6_ADDRSTRLEN> buffer;
	if (inet_ntop(AF_INET6, &ipv6.sin6_addr, buffer.data(), buffer.size()) != nullptr) {
		return buffer;
	} else {
		return std::nullopt;
	}
}

std::optional<std::array<char, INET6_ADDRSTRLEN>> any_ip_to_str(const any_ip_t& ip) {
	std::array<char, INET6_ADDRSTRLEN> buffer;
	if (ip.ss_family == AF_INET) {
		const auto& ipv4 = to_ipv4(ip);
		if (inet_ntop(AF_INET, &ipv4.sin_addr, buffer.data(), buffer.size()) != nullptr) {
			return buffer;
		}
	} else if (ip.ss_family == AF_INET6) {
		const auto& ipv6 = to_ipv6(ip);
		if (inet_ntop(AF_INET6, &ipv6.sin6_addr, buffer.data(), buffer.size()) != nullptr) {
			return buffer;
		}
	}
	return std::nullopt;
}

}

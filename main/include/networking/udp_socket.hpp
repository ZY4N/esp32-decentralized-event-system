#pragma once

#include <system_error>
#include <cstdint>
#include <span>
#include "util/uix.hpp"

#include "networking/socket_address.hpp"
#include "networking/safe_lwip_socket.hpp"
#include "lwip/sockets.h"


class udp_socket {
public:

	[[nodiscard]] std::error_code init();

	[[nodiscard]] std::error_code init_receiver(const socket_address::ipv4_t& address);
	[[nodiscard]] std::error_code init_receiver(const socket_address::ipv6_t& address);
	[[nodiscard]] std::error_code init_receiver(
		socket_address::const_ip_ptr_t address, socket_address::ip_size_t address_len
	);

	[[nodiscard]] std::error_code init_sender(const socket_address::ipv4_t& address);
	[[nodiscard]] std::error_code init_sender(const socket_address::ipv6_t& address);
	[[nodiscard]] std::error_code init_sender(
		socket_address::const_ip_ptr_t address, socket_address::ip_size_t address_len
	);

	[[nodiscard]] std::error_code set_broadcast(bool enabled);

	[[nodiscard]] std::error_code send(std::span<const ztu::u8> buffer);
	[[nodiscard]] std::error_code send(std::span<const ztu::u8> buffer, const socket_address::ipv4_t& address);
	[[nodiscard]] std::error_code send(std::span<const ztu::u8> buffer, const socket_address::ipv6_t& address);
	[[nodiscard]] std::error_code send(
		std::span<const ztu::u8> buffer, socket_address::const_ip_ptr_t address, socket_address::ip_size_t address_len
	);


	[[nodiscard]] std::error_code receive(std::span<ztu::u8>& buffer);
	[[nodiscard]] std::error_code receive(
		std::span<ztu::u8>& buffer, socket_address::any_ip_t& address, socket_address::ip_size_t& address_len
	);

private:
	safe_lwip_socket m_socket{};
};

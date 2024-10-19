#include "networking/udp_socket.hpp"

#include "lwip/err.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "util/make_system_error.hpp"


std::error_code udp_socket::init() {

	auto new_socket = safe_lwip_socket{ socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) };
	if (new_socket.fd < 0) {
		return make_system_error(errno);
	}

	m_socket = std::move(new_socket);

	return {};
}

std::error_code udp_socket::init_receiver(const socket_address::ipv4_t& address) {
	return init_receiver(socket_address::to_generic_ptr(address), sizeof(socket_address::ipv4_t));
}

std::error_code udp_socket::init_receiver(const socket_address::ipv6_t& address) {
	return init_receiver(socket_address::to_generic_ptr(address), sizeof(socket_address::ipv6_t));
}

std::error_code udp_socket::init_sender(const socket_address::ipv4_t& address) {
	return init_sender(socket_address::to_generic_ptr(address), sizeof(socket_address::ipv4_t));
}

std::error_code udp_socket::init_sender(const socket_address::ipv6_t& address) {
	return init_sender(socket_address::to_generic_ptr(address), sizeof(socket_address::ipv6_t));
}


std::error_code udp_socket::init_receiver(
	socket_address::const_ip_ptr_t address, const socket_address::ip_size_t address_len
) {

	if (auto error = init(); error) {
		return error;
	}

	if (auto ret = bind(m_socket.fd, address, address_len); ret < 0) {
		return make_system_error(errno);
	}

	return {};
}

std::error_code udp_socket::init_sender(
	socket_address::const_ip_ptr_t address, const socket_address::ip_size_t address_len
) {

	if (auto error = init(); error) {
		return error;
	}

	if (auto ret = connect(m_socket.fd, address, address_len); ret < 0) {
		return make_system_error(errno);
	}

	return {};
}

std::error_code udp_socket::set_broadcast(bool enabled) {

	int enabled_int = static_cast<int>(enabled);
	if (setsockopt(m_socket.fd, SOL_SOCKET, SO_BROADCAST, &enabled_int, sizeof(enabled_int)) < 0) {
		return make_system_error(errno);
	}

	return {};
}

std::error_code udp_socket::send(std::span<const ztu::u8> buffer) {
	const auto bytes_sent = ::send(m_socket.fd, buffer.data(), buffer.size(), 0);

	if (bytes_sent != buffer.size()) {
		return make_system_error(errno);
	}

	return {};
}

std::error_code udp_socket::send(std::span<const ztu::u8> buffer, const socket_address::ipv4_t& address) {
	return send(buffer, socket_address::to_generic_ptr(address), sizeof(socket_address::ipv4_t));
}

std::error_code udp_socket::send(std::span<const ztu::u8> buffer,  const socket_address::ipv6_t& address) {
	return send(buffer, socket_address::to_generic_ptr(address), sizeof(socket_address::ipv6_t));
}

std::error_code udp_socket::send(
	std::span<const ztu::u8> buffer, socket_address::const_ip_ptr_t address, const socket_address::ip_size_t address_len
) {

	const auto bytes_sent = sendto(
		m_socket.fd,
		buffer.data(), buffer.size(),
		0,
		address, address_len
	);

	if (bytes_sent != buffer.size()) {
		return make_system_error(errno);
	}

	return {};
}


std::error_code udp_socket::receive(std::span<ztu::u8>& buffer) {

	const auto bytes_received = recv(m_socket.fd, buffer.data(), buffer.size(), 0);

	if (bytes_received < 0) {
		return make_system_error(errno);
	}

	buffer = buffer.subspan(0, bytes_received);

	return {};
}

std::error_code udp_socket::receive(
	std::span<ztu::u8>& buffer, socket_address::any_ip_t& address, socket_address::ip_size_t& address_len
) {

	address_len = sizeof(socket_address::any_ip_t);

	const auto bytes_received = recvfrom(
		m_socket.fd,
		buffer.data(),
		buffer.size(),
		0,
		socket_address::to_generic_ptr(address),
		&address_len
	);

	if (bytes_received < 0) {
		return make_system_error(errno);
	}

	buffer = buffer.subspan(0, bytes_received);

	return {};
}

#include "networking/tcp_socket_connection.hpp"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "util/make_system_error.hpp"


std::error_code tcp_socket_connection::connect(const socket_address::ipv4_t& address) {
	return tcp_socket_connection::connect(
		socket_address::to_generic_ptr(address), sizeof(socket_address::ipv4_t)
	);
}

std::error_code tcp_socket_connection::connect(const socket_address::ipv6_t& address) {
	return tcp_socket_connection::connect(
		socket_address::to_generic_ptr(address), sizeof(socket_address::ipv6_t)
	);
}

std::error_code tcp_socket_connection::connect(
	socket_address::const_ip_ptr_t address, const socket_address::ip_size_t address_len
) {
	disconnect();

	safe_lwip_socket new_socket{ ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };
	if (new_socket.fd < 0) {
		return make_system_error(errno);
	}

	if (::connect(new_socket.fd, address, address_len) != 0) {
		return make_system_error(errno);
	}

	m_socket = std::move(new_socket);
	
	return {};
}

std::error_code tcp_socket_connection::send(std::span<const ztu::u8>& bytes_left) {
	while (not bytes_left.empty()) {
		const auto sent = ::send(m_socket.fd, bytes_left.data(), bytes_left.size(), 0);
		if (sent <= 0) return make_system_error(errno);
		bytes_left = bytes_left.subspan(sent);
	}
	return {};
}

std::error_code tcp_socket_connection::receive(std::span<ztu::u8>& bytes_left) {
	while (not bytes_left.empty()) {
		const auto received = recv(m_socket.fd, bytes_left.data(), bytes_left.size(), 0);
		if (received <= 0 and errno != EINTR) return make_system_error(errno);
		bytes_left = bytes_left.subspan(received);
	}
	return {};
}

void tcp_socket_connection::disconnect() {
	if (m_socket.fd >= 0) {
		shutdown(m_socket.fd, SHUT_RDWR);
	}
}


[[nodiscard]] std::error_code tcp_socket_connection::set_send_timeout(const ztu::u32 milliseconds) {
	timeval timeout_interval {
		.tv_sec = milliseconds / 1000,
		.tv_usec = static_cast<suseconds_t>((milliseconds % 1000) * 1000)
	};
	if (setsockopt(m_socket.fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout_interval), sizeof(timeout_interval)) != 0) {
		return make_system_error(errno);
	}
	return {};
}

[[nodiscard]] std::error_code tcp_socket_connection::set_receive_timeout(const ztu::u32 milliseconds) {
	timeval timeout_interval {
		.tv_sec = milliseconds / 1000,
		.tv_usec = static_cast<suseconds_t>((milliseconds % 1000) * 1000)
	};
	if (setsockopt(
		m_socket.fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_interval), sizeof(timeout_interval)
	) != 0) {
		return make_system_error(errno);
	}
	return {};
}

safe_lwip_socket &tcp_socket_connection::socket() {
	return m_socket;
}

tcp_socket_connection::tcp_socket_connection(tcp_socket_connection&& other) {
	m_socket = std::move(other.m_socket);
}

tcp_socket_connection& tcp_socket_connection::operator=(tcp_socket_connection&& other) {
	if (&other != this) {
		disconnect();
		m_socket = std::move(other.m_socket);
	}
	return *this;
}

tcp_socket_connection::~tcp_socket_connection() {
	disconnect();
}

#include "networking/tcp_socket_acceptor.hpp"

#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "networking/socket_address.hpp"
#include "util/make_system_error.hpp"


std::error_code tcp_socket_acceptor::init(const socket_address::ipv4_t& address, int connection_queue_size) {
	return init(socket_address::to_generic_ptr(address), sizeof(socket_address::ipv4_t), connection_queue_size);
}

std::error_code tcp_socket_acceptor::init(const socket_address::ipv6_t& address, int connection_queue_size) {
	return init(socket_address::to_generic_ptr(address), sizeof(socket_address::ipv6_t), connection_queue_size);
}

std::error_code tcp_socket_acceptor::init(
	socket_address::const_ip_ptr_t address,
	const socket_address::ip_size_t address_len,
	int connection_queue_size
) {

	safe_lwip_socket listen_sock{ socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };

	if (listen_sock.fd < 0) {
		return make_system_error(errno);
	}

	int opt = 1;
	if (setsockopt(listen_sock.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
		return make_system_error(errno);
	}

	if (bind(listen_sock.fd, address, address_len) != 0) {
		return make_system_error(errno);
	}

	if (::listen(listen_sock.fd, connection_queue_size) != 0) {
		return make_system_error(errno);
	}

	m_socket = std::move(listen_sock);

	return {};
}

std::error_code tcp_socket_acceptor::listen(
	tcp_socket_connection &connection,
	socket_address::any_ip_t& address,
	socket_address::ip_size_t& address_len,
	ztu::u32 timeout_ms
 ) {

	connection.disconnect();

	if (timeout_ms) {
		timeval timeout_interval {
			.tv_sec = timeout_ms / 1000,
			.tv_usec = static_cast<suseconds_t>((timeout_ms % 1000) * 1000)
		};

		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(m_socket.fd, &read_fds);

		const auto ret = select(m_socket.fd + 1, &read_fds, nullptr, nullptr, &timeout_interval);
		if (ret <  0) return make_system_error(errno);
		if (ret == 0) return make_system_error(EAGAIN);
	}

	address_len = sizeof(socket_address::any_ip_t);

	safe_lwip_socket sock{ accept(m_socket.fd, socket_address::to_generic_ptr(address), &address_len) };
	if (sock.fd < 0) {
		return std::error_code(errno, std::system_category());
	}
		
	connection.socket() = std::move(sock);

	return {};
}

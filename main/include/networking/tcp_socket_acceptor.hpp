#pragma once

#include <system_error>

#include "networking/tcp_socket_connection.hpp"
#include "networking/socket_address.hpp"

/**
 * @class socket_acceptor
 * 
 * @brief A class that represents a socket acceptor.
 * 
 * This class provides a simple interface for accepting incoming connections and initializing a `socket_connection` instance.
 * It provides a `initialize` method for creating a socket with the specified address family and port,
 * and a `listen` method for waiting for incoming connections and initializing a `socket_connection` instance.
 */
class tcp_socket_acceptor {
public:
	static constexpr auto default_connection_queue_size = 1;

	tcp_socket_acceptor() = default;

	[[nodiscard]] std::error_code init(
		const socket_address::ipv4_t& address,
		int connection_queue_size = default_connection_queue_size
	);

	[[nodiscard]] std::error_code init(
		const socket_address::ipv6_t& address,
		int connection_queue_size = default_connection_queue_size
	);

	[[nodiscard]] std::error_code init(
		socket_address::const_ip_ptr_t address,
		socket_address::ip_size_t address_len,
		int connection_queue_size = default_connection_queue_size
	);
	
	/**
	 * @brief This function waits for an incoming connection and initializes the given 'socket_connection` instance.
	 *
	 * @param connection `socket_connection` instance to store the newly created socket in.
	 *
	 * @return std::error_code indicating the result of the operation. Zero on success, non-zero on error.
	 */
	[[nodiscard]] std::error_code listen(
		tcp_socket_connection& connection,
		socket_address::any_ip_t& address,
		socket_address::ip_size_t& address_len,
		ztu::u32 timeout_ms = 0
	);

private:
	


private:
	safe_lwip_socket m_socket{};
};

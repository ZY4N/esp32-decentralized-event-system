#pragma once

#include "networking/safe_lwip_socket.hpp"

#include <system_error>
#include <span>

#include "util/uix.hpp"
#include "networking/socket_address.hpp"

/**
 * @class tcp_socket_connection
 *
 * @brief Represents a socket connection for sending and receiving data.
 *
 * The `tcp_socket_connection` class provides a convenient interface for working with sockets. The class provides
 * methods for sending and receiving data over a connected socket, as well as for disconnecting the socket connection.
 * The socket connection is managed using a `safe_fd` object, which is responsible for managing the underlying file
 * descriptor associated with the socket.
 *
 * @note This class is not responsible for creating the socket or establishing the connection.
 */
class tcp_socket_connection {
public:
	tcp_socket_connection() = default;

	tcp_socket_connection(const tcp_socket_connection&) = delete;
	tcp_socket_connection& operator=(const tcp_socket_connection&) = delete;

	tcp_socket_connection(tcp_socket_connection&& other);
	tcp_socket_connection& operator=(tcp_socket_connection&& other);


	[[nodiscard]] std::error_code connect(const socket_address::ipv4_t& address);

	[[nodiscard]] std::error_code connect(const socket_address::ipv6_t& address);

	[[nodiscard]] std::error_code connect(
		socket_address::const_ip_ptr_t address,
		socket_address::ip_size_t address_len
	);

	[[nodiscard]] std::error_code connect(const socket_address::ipv4_t& address, ztu::u32 timeout_ms);

	[[nodiscard]] std::error_code connect(const socket_address::ipv6_t& address, ztu::u32 timeout_ms);

	[[nodiscard]] std::error_code connect(
		socket_address::const_ip_ptr_t address,
		socket_address::ip_size_t address_len,
		ztu::u32 timeout_ms
	);

	/**
	 * @brief Sends data over a connected socket.
	 *
	 * @param buffer The data to send, represented as a span of unsigned 8-bit integers.
	 *
	 * @return A standard error code that indicates the result of the operation. If the operation was successful,
	 * the error code will have a value of 0. Otherwise, the error code will contain a non-zero value indicating the
	 * error that occurred. The error code is accompanied by a system category to provide more context about the error.
	 */
	[[nodiscard]] std::error_code send(std::span<const ztu::u8>& data);


	/**
	 * @brief Receives data from a connected socket.
	 *
	 * @param buffer The buffer to store the received data, represented as a span of unsigned 8-bit integers.
	 *
	 * @return A standard error code that indicates the result of the operation. If the operation was successful,
	 * the error code will have a value of 0. Otherwise, the error code will contain a non-zero value indicating the
	 * error that occurred. The error code is accompanied by a system category to provide more context about the error.
	 */
	[[nodiscard]] std::error_code receive(std::span<ztu::u8>& data);


	/**
     * @brief Disconnects the socket connection.
	 * 
     * If the socket file descriptor is valid this function disconnects the socket connection
	 * by shutting down both reading and writing channels.
	 *
     * @note This function does not close the socket file descriptor itself.
    */
	void disconnect();


	safe_lwip_socket &socket();

	[[nodiscard]] std::error_code set_send_timeout(ztu::u32 milliseconds);

	[[nodiscard]] std::error_code set_receive_timeout(ztu::u32 milliseconds);

	~tcp_socket_connection();

private:
	safe_lwip_socket m_socket{};
};

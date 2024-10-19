#pragma once

#include "lwip/sockets.h"


struct safe_lwip_socket {

	static constexpr int invalid_fd = -1;

	safe_lwip_socket() = default;

	inline safe_lwip_socket(int fileDescriptor);
	inline safe_lwip_socket(safe_lwip_socket&& other);
	inline safe_lwip_socket &operator=(safe_lwip_socket&& other);

	// Don't allow sockets to be copied
	safe_lwip_socket(const safe_lwip_socket& other) = delete;
	safe_lwip_socket& operator=(const safe_lwip_socket& other) = delete; 

	inline ~safe_lwip_socket();

	int fd{ invalid_fd };
};


safe_lwip_socket::safe_lwip_socket(int fileDescriptor) : fd{ fileDescriptor } {}

safe_lwip_socket::safe_lwip_socket(safe_lwip_socket&& other) : fd{ other.fd } {
	other.fd = invalid_fd;
}

safe_lwip_socket& safe_lwip_socket::operator=(safe_lwip_socket&& other) {
	if (&other != this) {
		this->~safe_lwip_socket();
		fd = other.fd;
		other.fd = invalid_fd;
	}
	return *this;
}

safe_lwip_socket::~safe_lwip_socket() {
	if (fd >= 0) {
		close(fd);
	}
}

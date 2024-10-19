#ifndef INCLUDE_NETWORK_EVENT_HANDLER_IMPLEMENTATION
#error Never include this file directly include 'network_event_handler.hpp'
#endif

#include "esp_random.h"
#include <algorithm>
#include "crypto/signature_engine_error.hpp"
#include "util/feed_the_beast.hpp"

template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
std::error_code network_event_handler<Event, TrustedCount>::init(
	std::span<const ztu::u8> private_key,
	const std::array<std::array<ztu::u8, signature_engine::public_key_size>, TrustedCount>& public_keys,
	const sender_id_t sender_id
) {
	using namespace network_event_handler_error;

	this->~network_event_handler();

	m_sender_id = sender_id;
	m_public_keys = public_keys;

	feed_the_beast();

	if (const auto e = m_signator.init(private_key); e)
		return e;

	feed_the_beast();

	return {};
}


template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
std::error_code network_event_handler<Event, TrustedCount>::set_mode(mode new_mode) {
	if (new_mode != m_current_mode) {

		std::error_code error;

		switch (new_mode) {
			case mode::sender:
				feed_the_beast();

				if ((error = m_event_socket.init_sender(socket_address::broadcast_ipv4(CONFIG_BROADCAST_PORT))))
					return error;

				feed_the_beast();

				if ((error = m_event_socket.set_broadcast(true)))
					return error;

				feed_the_beast();

				if ((error = m_challenge_socket_acceptor.init(socket_address::any_ipv4(CONFIG_CHALLENGE_PORT), TrustedCount)))
					return error;

				feed_the_beast();
				
				break;
			case mode::receiver:
				feed_the_beast();

				if ((error = m_event_socket.init_receiver(socket_address::any_ipv4(CONFIG_BROADCAST_PORT))))
					return error;

				feed_the_beast();

				if ((error = m_event_socket.set_broadcast(true)))
					return error;

				feed_the_beast();
				break;
			default:
				m_event_socket.~udp_socket();
				m_challenge_socket_acceptor.~tcp_socket_acceptor();
				m_challenge_socket.~tcp_socket_connection();
				break;
		}

		m_current_mode = new_mode;
	}

	return {};
}

template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
std::error_code network_event_handler<Event, TrustedCount>::send(Event event, ztu::u32 max_retries) {

	using namespace network_event_handler_error;
	codes ec;

	if (const auto e = set_mode(mode::sender); e) {
		return e;
	}

	const auto event_id = static_cast<event_id_t>(event);

	auto tries_left = 1 + max_retries;

	do {
		if (is_error(ec = send_event(event_id, 10))) { // TODO find better number
			switch (ec) {
				using enum codes;
				case ran_out_of_tries:
					continue;
				default:
					return make_error_code(ec);
			}
		}
		

		// Wait for challenges
		for (ztu::usize i{}; i <= TrustedCount; ++i) {

			socket_address::any_ip_t address;
			socket_address::ip_size_t address_len;

			if (is_error(ec = listen_for_challengers(address, address_len, CONFIG_TCP_LISTEN_TIMEOUT_MS))) {
				switch (ec) {
					using enum codes;
					case temporary_listening_error:
						continue;
					default:
						return make_error_code(ec);
				}
			}

			const auto ip_str_opt = socket_address::any_ip_to_str(address);
			ESP_LOGI(TAG, "Receiving challenge from %s",
				ip_str_opt ? ip_str_opt.value().data() : "<malformed address>"
			);

			nonce_t challenge_nonce;
			if (is_error(ec = receive_challenge(event_id, challenge_nonce, CONFIG_TCP_COMMUNICATON_RETRIES))) {
				switch (ec) {
					using enum codes;
					case challenge_connection_broken:
					case ran_out_of_tries:
					case invalid_challenge_request:
						continue;
					default:
						return make_error_code(ec);
				}
			}
			
			if (is_error(ec = send_signature(event_id, challenge_nonce, CONFIG_TCP_COMMUNICATON_RETRIES))) {
				switch (ec) {
					using enum codes;
					case challenge_connection_broken:
					case ran_out_of_tries:
						continue;
					default:
						return make_error_code(ec);
				}
			}

			ESP_LOGI(TAG, "Successfully answered challenge!");
		}
	} while (--tries_left);

	return codes::ok;
}



template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
std::error_code network_event_handler<Event, TrustedCount>::await(std::span<const Event> events, Event& received_event, ztu::u32 max_retries) {
	
	if (const auto e = set_mode(mode::receiver); e) {
		return e;
	}

	using namespace network_event_handler_error;
	codes ec;

	socket_address::any_ip_t sender_address;
	socket_address::ip_size_t sender_address_len;
	event_id_t event_id;

try_receive_event:
	if (is_error(ec = receive_event(events, event_id, sender_address, sender_address_len, 10))) { // TODO find better number
		switch (ec) {
			using enum codes;
			case ran_out_of_tries:
			case invalid_event_message:
				goto try_receive_event;
			default:
				return make_error_code(ec);
		}
	}

	const auto ip_str_opt = socket_address::any_ip_to_str(sender_address);
	ESP_LOGI(TAG, "Sending challenge to %s",
		ip_str_opt ? ip_str_opt.value().data() : "<malformed address>"
	);

	// Change the sender port from the udp to the tcp port.
	const auto tcp_port = htons(CONFIG_CHALLENGE_PORT);
	if (socket_address::is_ipv4(sender_address)) {
		socket_address::to_ipv4(sender_address).sin_port = tcp_port;
	} else {
		socket_address::to_ipv6(sender_address).sin6_port = tcp_port;
	}
	
try_connect_to_sender:
	if (is_error(ec = connect_to_sender(sender_address, sender_address_len, TrustedCount))) {
		switch (ec) {
			using enum codes;
			case cannot_connect_to_sender:
			case ran_out_of_tries:
				goto try_receive_event;
			default:
				return make_error_code(ec);
		}
	}

	nonce_t challenge_nonce;
	esp_fill_random(challenge_nonce.data(), challenge_nonce.size());

	if (is_error(ec = send_challenge(event_id, challenge_nonce, CONFIG_TCP_COMMUNICATON_RETRIES))) {
		switch (ec) {
			using enum codes;
			case challenge_connection_broken:
				goto try_connect_to_sender;
			case ran_out_of_tries:
				goto try_receive_event;
			default:
				return make_error_code(ec);

		}
	}

	if (is_error(ec = receive_signature(event_id, challenge_nonce, CONFIG_TCP_COMMUNICATON_RETRIES))) {
		switch (ec) {
			using enum codes;
			case challenge_connection_broken:
				goto try_connect_to_sender;
			case invalid_challenge_response:
				goto try_receive_event;
			default:
				return make_error_code(ec);
		}
	}

	return {};
}


template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
network_event_handler_error::codes network_event_handler<Event, TrustedCount>::send_event(
	event_id_t event_id, const ztu::u32 max_retries
) {

	using namespace network_event_handler_error;
	std::error_code error;

	const auto msg_buffer =
		basic_codec::based_on<msg_event_t>::serialize(
			msg_event_type,
			event_id
		);

	return broadcast_send(msg_buffer, max_retries);
}

template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
network_event_handler_error::codes network_event_handler<Event, TrustedCount>::receive_event(
	std::span<const Event> events, event_id_t& event_id,
	socket_address::any_ip_t& sender_address,
	socket_address::ip_size_t& sender_address_len,
	const ztu::u32 max_retries
) {

	using namespace network_event_handler_error;

	basic_codec::buffer_t<msg_event_t> msg_buffer{};
	if (const auto ec = broadcast_receive(msg_buffer, sender_address, sender_address_len, max_retries); is_error(ec)) {
		return ec;
	}
	
	message_type_t msg_type;

	basic_codec::based_on<msg_event_t>::deserialize(
		msg_buffer,
		msg_type,
		event_id
	);

	if (msg_type != msg_event_type) {
		return codes::invalid_event_message;
	}

	if (std::find(events.begin(), events.end(), static_cast<Event>(event_id)) == events.end()) {
		return codes::invalid_event_message;
	}

	return codes::ok;
}

template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
network_event_handler_error::codes network_event_handler<Event, TrustedCount>::send_challenge(
	event_id_t event_id, nonce_t& nonce,
	const ztu::u32 max_retries
) {
	using namespace network_event_handler_error;

	const auto msg_buffer = basic_codec::based_on<msg_challenge_t>::serialize(
		msg_challenge_type,
		event_id,
		nonce
	);

	return challenge_socket_send(msg_buffer, max_retries);
}

template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
network_event_handler_error::codes network_event_handler<Event, TrustedCount>::receive_challenge(
	event_id_t event_id,
	nonce_t& challenge_nonce,
	const ztu::u32 max_retries
) {
	using namespace network_event_handler_error;

	basic_codec::buffer_t<msg_challenge_t> msg_buffer{};
	if (const auto ec = challenge_socket_receive(msg_buffer, max_retries); is_error(ec)) {
		return ec;
	}

	message_type_t challenge_type;
	event_id_t challenge_event_id;
	
	basic_codec::based_on<msg_challenge_t>::deserialize(
		msg_buffer,
		challenge_type,
		challenge_event_id,
		challenge_nonce
	);

	if (challenge_type != msg_challenge_type) {
		return codes::invalid_challenge_request;
	}

	if (challenge_event_id != event_id) {
		return codes::invalid_challenge_request;
	}

	return codes::ok;
}

template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
network_event_handler_error::codes network_event_handler<Event, TrustedCount>::send_signature(
	const event_id_t event_id, const nonce_t& challenge_nonce,
	const ztu::u32 max_retries
) {
	using namespace network_event_handler_error;

	auto msg_buffer =
		basic_codec::based_on<msg_signature_t>::serialize(
			msg_signature_type,
			event_id,
			challenge_nonce,
			m_sender_id,
			{}
		);

	auto msg_body = std::span{
		msg_buffer.begin(), 
		msg_buffer.end() - basic_codec::serialized_size_v<signature_t>
	};

	signature_t signature{};

	std::error_code error;

	feed_the_beast();
	if ((error = m_signator.sign(msg_body, signature))) {

		log_error_code(error);

		return codes::internal_signing_error;
	}

	basic_codec::based_on<msg_signature_t>::template serialize_at<4>(
		signature, msg_buffer
	);

	message_type_t msg_type;
	event_id_t msg_event_id;
	nonce_t msg_nonce;
	sender_id_t msg_sender_id;
	signature_t msg_signature;
	
	basic_codec::based_on<msg_signature_t>::deserialize(
		msg_buffer,
		msg_type,
		msg_event_id,
		msg_nonce,
		msg_sender_id,
		msg_signature
	);
	
	return challenge_socket_send(msg_buffer, max_retries);
}


template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
network_event_handler_error::codes network_event_handler<Event, TrustedCount>::receive_signature(
	event_id_t event_id, const nonce_t& challenge_nonce,
	const ztu::u32 max_retries
) {
	using namespace network_event_handler_error;

	basic_codec::buffer_t<msg_signature_t> msg_buffer{};
	if (const auto ec = challenge_socket_receive(msg_buffer, max_retries); is_error(ec)) {
		return ec;
	}
	
	message_type_t msg_type;
	event_id_t msg_event_id;
	nonce_t msg_nonce;
	sender_id_t msg_sender_id;
	signature_t msg_signature;
	
	basic_codec::based_on<msg_signature_t>::deserialize(
		msg_buffer,
		msg_type,
		msg_event_id,
		msg_nonce,
		msg_sender_id,
		msg_signature
	);

	if (msg_type != msg_signature_type) {
		return codes::invalid_challenge_response;
	}

	if (msg_event_id != event_id) {
		return codes::invalid_challenge_response;
	}

	if (msg_nonce != challenge_nonce) {
		return codes::invalid_challenge_response;
	}

	if (msg_sender_id >= TrustedCount) {
		return codes::invalid_challenge_response;
	}

	if (m_signator.set_verification_key(m_public_keys[msg_sender_id])) {
		return codes::internal_verification_error;
	}

	const auto msg_body = std::span{
		msg_buffer.begin(),
		msg_buffer.end() - basic_codec::serialized_size_v<signature_t>
	};

	std::error_code error;
	bool signature_correct;

	feed_the_beast();
	if ((error = m_signator.verify(msg_body, msg_signature, signature_correct))) {

		log_error_code(error);

		if (error.category() == signature_engine_category()) {
			switch (static_cast<signature_engine_error::codes>(error.value())) {
				using enum signature_engine_error::codes;
				case signature_extraction_failed:
				case decryption_failed:
					return codes::invalid_challenge_response;
				default:
					return codes::internal_verification_error;
			}
		} else {
			return codes::internal_verification_error;
		}
	}

	if (not signature_correct) {
		return codes::invalid_challenge_response;
	}

	return codes::ok;
}


template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
network_event_handler_error::codes network_event_handler<Event, TrustedCount>::listen_for_challengers(
	socket_address::any_ip_t& address,
	socket_address::ip_size_t& address_len,
	const ztu::u32 timeout_ms
) {
	using namespace network_event_handler_error;
	std::error_code error;

	feed_the_beast();
	if ((error = m_challenge_socket_acceptor.listen(
		m_challenge_socket, address, address_len, timeout_ms))
	) {

		log_error_code(error);

		if (error.category() == std::generic_category()) {
			switch (error.value()) {
				case EINTR:			// interrupted by signal
				case EAGAIN:		// reached timeout
				case ECONNABORTED:	// connection aborted
				case EMFILE:		// cannot open more fds
				case ENOBUFS:		// Not enough memory
				case ENOMEM:		// "
				case EPROTO:		// protocol error
					// Issue might be resolved in next retry.
					return codes::temporary_listening_error;
				default:
					// internal error
					return codes::internal_listening_error;
			}
		} else {
			return codes::internal_listening_error;
		}
	}

	return codes::ok;
}

template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
network_event_handler_error::codes network_event_handler<Event, TrustedCount>::connect_to_sender(
	const socket_address::any_ip_t& address,
	const socket_address::ip_size_t address_len,
	const ztu::u32 max_retries
) {
	using namespace network_event_handler_error;
	std::error_code error;

	auto tries_left = 1 + max_retries;

	do {
		feed_the_beast();
		if ((error = m_challenge_socket.connect(socket_address::to_generic_ptr(address), address_len))) {
			
			log_error_code(error);

			if (error.category() == std::generic_category()) {
				switch (error.value()) {
					case EINTR:			// signal interrupted operation
					case EAGAIN:		// routing cache issue
					case ECONNREFUSED:	// reseiver either not (yet?) listening
					case ETIMEDOUT:		// reseiver too busy
					case ENETUNREACH:	// network unreachable
					case ECONNRESET:	// peer reset connection
						// Issue might be resolved in next retry.
						break;
					case EPERM:			// broadcast not enabled
					case EADDRINUSE:	// address already in use
					case EADDRNOTAVAIL:	// address not set
					case EAFNOSUPPORT:	// address malformed
					case EFAULT:		// address outside of devices address space
						// Issue will be persistant for this peer/address
						return codes::cannot_connect_to_sender;
					default:
						// internal error
						return codes::internal_connecting_error;
				}
			} else {
				return codes::internal_connecting_error;
			}
		} else {
			break; // Success!
		}
	} while (--tries_left);

	if (not tries_left)
		return codes::ran_out_of_tries;

	return codes::ok;
}

template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
template<ztu::usize BufferSize>
network_event_handler_error::codes network_event_handler<Event, TrustedCount>::broadcast_send(
	const std::array<ztu::u8, BufferSize>& msg_buffer,
	const ztu::u32 max_retries
) {
	using namespace network_event_handler_error;
	std::error_code error;

	auto tries_left = 1 + max_retries;

	do {
		feed_the_beast();
		if ((error = m_event_socket.send(msg_buffer))) {

			log_error_code(error);

			if (error.category() == std::generic_category()) {
				switch (error.value()) {
					case EINTR:			// signal interrupted operation
					case EAGAIN:		// routing cache issue
					case ENETUNREACH:	// network unreachable
						// Issue might be resolved in next retry.
						break;
					case EAFNOSUPPORT:	// address malformed
						// Issue will be persistant for this peer/address
						return codes::cannot_connect_to_broadcast;
					default:
						return codes::internal_broadcast_sending_error;
				}
			} else {
				return codes::internal_broadcast_sending_error;
			}
		} else {
			break; // Success!
		}
	} while (--tries_left);

	if (not tries_left)
		return codes::ran_out_of_tries;

	return codes::ok;
}

template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
template<ztu::usize BufferSize>
network_event_handler_error::codes network_event_handler<Event, TrustedCount>::broadcast_receive(
	std::array<ztu::u8, BufferSize>& msg_buffer,
	socket_address::any_ip_t& address,
	socket_address::ip_size_t& address_len,
	const ztu::u32 max_retries
) {
	using namespace network_event_handler_error;
	std::error_code error;

	auto tries_left = 1 + max_retries;

	do {
		auto to_be_received = std::span<ztu::u8, std::dynamic_extent>{ msg_buffer };

		feed_the_beast();
		if ((error = m_event_socket.receive(to_be_received, address, address_len))) {

			log_error_code(error);

			if (error.category() == std::generic_category()) {
				switch (error.value()) {
					case EINTR:			// signal interrupted operation
					case ENOBUFS:		// buffers full
					case ENOMEM:		// out of memory
					case ETIMEDOUT:		// timed out while waiting for connection
						// Issue might be resolved in next retry.
						break;
					default:
						return codes::internal_broadcast_receiving_error;
				}
			} else {
				return codes::internal_broadcast_receiving_error;
			}
		} else if (to_be_received.size() == msg_buffer.size()) {
			break;
		}
	} while (--tries_left);

	if (not tries_left)
		return codes::ran_out_of_tries;

	return codes::ok;
}


template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
template<ztu::usize BufferSize>
network_event_handler_error::codes network_event_handler<Event, TrustedCount>::challenge_socket_send(
	const std::array<ztu::u8, BufferSize>& msg_buffer, const ztu::u32 max_retries
) {
	using namespace network_event_handler_error;
	std::error_code error;
	
	auto tries_left = 1 + max_retries;

	auto to_be_sent = std::span<const ztu::u8, std::dynamic_extent>{ msg_buffer };

	do {
		feed_the_beast();
		if ((error = m_challenge_socket.send(to_be_sent))) {

			log_error_code(error);

			if (error.category() == std::generic_category()) {
				switch (error.value()) {
					case EINTR:			// signal interrupted operation
					case ENOBUFS:		// buffers full
					case ENOMEM:		// out of memory
						// Issue might be resolved in next retry.
						break;
					case ECONNRESET:	// peer closed connection
					case EPIPE:			// device closed connection
						// Issue will be persistant for this connection
						return codes::challenge_connection_broken;
					default:
						// Some internal error
						return codes::internal_sending_error;
				}
			} else {
				return codes::internal_sending_error;
			}
		} else {
			break; // Success!
		}
	} while (not to_be_sent.empty() && --tries_left);

	if (not tries_left)
		return codes::ran_out_of_tries;

	return codes::ok;
}

template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
template<ztu::usize BufferSize>
network_event_handler_error::codes network_event_handler<Event, TrustedCount>::challenge_socket_receive(
	std::array<ztu::u8, BufferSize>& msg_buffer, const ztu::u32 max_retries
) {
	using namespace network_event_handler_error;
	std::error_code error;

	auto tries_left = 1 + max_retries;

	auto to_be_received = std::span<ztu::u8, std::dynamic_extent>{ msg_buffer };

	do {
		feed_the_beast();
		if ((error = m_challenge_socket.receive(to_be_received))) {

			log_error_code(error);

			if (error.category() == std::generic_category()) {
				switch (error.value()) {
					case EINTR:			// signal interrupted operation
					case ENOBUFS:		// buffers full
					case ENOMEM:		// out of memory
						// Issue might be resolved in next receive
						break;
					case ECONNRESET:	// peer closed connection
					case EPIPE:			// device closed connection
						return codes::challenge_connection_broken;
					default:
						return codes::internal_receiving_error;
				}
			} else {
				return codes::internal_receiving_error;
			}
		} else {
			break; // Success!
		}
	} while (not to_be_received.empty() && --tries_left);

	if (not tries_left)
		return codes::ran_out_of_tries;

	return codes::ok;	
}

template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
void network_event_handler<Event, TrustedCount>::log_error_code(
	const std::error_code &e
) {
	ESP_LOGE(TAG, "[%s]: %s", e.category().name(), e.message().c_str());
}

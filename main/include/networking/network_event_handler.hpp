

#include <system_error>
#include <span>
#include <tuple>
#include <array>

#include "util/uix.hpp"
#include "crypto/signature_engine.hpp"
#include "networking/udp_socket.hpp"
#include "networking/tcp_socket_acceptor.hpp"
#include "networking/tcp_socket_connection.hpp"
#include "networking/basic_codec.hpp"

#include "networking/network_event_handler_error.hpp"

template<typename Event, ztu::usize TrustedCount>
	requires std::is_enum_v<Event>
struct network_event_handler {

private:
	enum class mode {
		none,
		sender,
		receiver
	};

	using message_type_t = ztu::u8;
	using event_id_t = std::underlying_type_t<Event>;
	using sender_id_t = ztu::u8;
	using nonce_t = std::array<ztu::u8, 16>;
	using signature_t = std::array<ztu::u8, signature_engine::signature_size>;

public:

	[[nodiscard]] inline std::error_code init(
		std::span<const ztu::u8> private_key,
		const std::array<std::array<ztu::u8, signature_engine::public_key_size>, TrustedCount>& public_keys,
		sender_id_t sender_id
	);

	[[nodiscard]] inline std::error_code send(Event event, ztu::u32 max_retries);

	[[nodiscard]] inline std::error_code await(std::span<const Event> events, Event& received_event, ztu::u32 max_retries);



protected:
	static inline constexpr auto TAG = "network_event_handler";

	static constexpr message_type_t
		msg_event_type = 0,
		msg_challenge_type = 1,
		msg_signature_type = 3;


	using msg_event_t = std::tuple<
		message_type_t,
		event_id_t
	>;

	using msg_challenge_t = std::tuple<
		message_type_t,
		event_id_t,
		nonce_t
	>;

	using msg_signature_t = std::tuple<
		message_type_t,
		event_id_t,
		nonce_t,
		sender_id_t,
		signature_t
	>;

protected:
	[[nodiscard]] std::error_code set_mode(mode new_mode);

	[[nodiscard]] inline network_event_handler_error::codes send_event(
		event_id_t event_id, ztu::u32 max_retries
	);

	[[nodiscard]] inline network_event_handler_error::codes receive_event(
		std::span<const Event> events, event_id_t& event_id,
		socket_address::any_ip_t& sender_address,
		socket_address::ip_size_t& sender_address_len,
		ztu::u32 max_retries
	);

	[[nodiscard]] inline network_event_handler_error::codes send_challenge(
		event_id_t event_id, nonce_t& nonce,
		ztu::u32 max_retries
	);

	[[nodiscard]] inline network_event_handler_error::codes receive_challenge(
		event_id_t event_id, nonce_t& challenge_nonce,
		ztu::u32 max_retries
	);

	[[nodiscard]] inline network_event_handler_error::codes send_signature(
		const event_id_t event_id, const nonce_t& challenge_nonce,
		ztu::u32 max_retries
	);

	[[nodiscard]] inline network_event_handler_error::codes receive_signature(
		event_id_t event_id, const nonce_t& challenge_nonce,
		ztu::u32 max_retries
	);

	[[nodiscard]] inline network_event_handler_error::codes listen_for_challengers(
		socket_address::any_ip_t& address,
		socket_address::ip_size_t& address_len,
		ztu::u32 timeout_ms
	);

	[[nodiscard]] inline network_event_handler_error::codes connect_to_sender(
		const socket_address::any_ip_t& address,
		const socket_address::ip_size_t address_len,
		ztu::u32 max_retries
	);

	template<ztu::usize BufferSize>
	[[nodiscard]] inline network_event_handler_error::codes broadcast_send(
		const std::array<ztu::u8, BufferSize>& msg_buffer,
		ztu::u32 max_retries
	);

	template<ztu::usize BufferSize>
	[[nodiscard]] inline network_event_handler_error::codes broadcast_receive(
		std::array<ztu::u8, BufferSize>& msg_buffer,
		socket_address::any_ip_t& address,
		socket_address::ip_size_t& address_len,
		ztu::u32 max_retries
	);

	template<ztu::usize BufferSize>
	[[nodiscard]] inline network_event_handler_error::codes challenge_socket_send(
		const std::array<ztu::u8, BufferSize>& msg_buffer,
		ztu::u32 max_retries
	);

	template<ztu::usize BufferSize>
	[[nodiscard]] inline network_event_handler_error::codes challenge_socket_receive(
		std::array<ztu::u8, BufferSize>& msg_buffer,
		ztu::u32 max_retries
	);

	static void inline log_error_code(const std::error_code &e);

private:
	signature_engine m_signator{};
	udp_socket m_event_socket{};
	tcp_socket_acceptor m_challenge_socket_acceptor{};
	tcp_socket_connection m_challenge_socket{};

	std::array<std::array<ztu::u8, signature_engine::public_key_size>, TrustedCount> m_public_keys;
	sender_id_t m_sender_id;

	mode m_current_mode{ mode::none };
};

#define INCLUDE_NETWORK_EVENT_HANDLER_IMPLEMENTATION
#include "networking/network_event_handler.ipp"
#undef INCLUDE_NETWORK_EVENT_HANDLER_IMPLEMENTATION

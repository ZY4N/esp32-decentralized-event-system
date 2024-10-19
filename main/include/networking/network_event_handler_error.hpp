#pragma once

#include <system_error>

namespace network_event_handler_error {
	
enum class codes : int {
	ok = 0,

	wrong_number_of_public_keys,
	wrong_public_key_size,

	temporary_listening_error,
	cannot_connect_to_broadcast,
	cannot_connect_to_sender,
	challenge_connection_broken,

	invalid_event_message,
	invalid_challenge_request,
	invalid_challenge_response,

	internal_listening_error,
	internal_connecting_error,
	internal_sending_error,
	internal_broadcast_sending_error,
	internal_receiving_error,
	internal_broadcast_receiving_error,
	internal_signing_error,
	internal_verification_error,
	
	ran_out_of_tries
};

inline bool is_ok(const codes code) {
	return code == codes::ok;
}

inline bool is_error(const codes code) {
	return code != codes::ok;
}

struct category : std::error_category {
	const char* name() const noexcept override {
		return "network_event_handler";
	}
	std::string message(int ev) const override {
		switch (static_cast<codes>(ev)) {
			using enum codes;
		case ok:
			return "Operation successful.";
		case wrong_number_of_public_keys:
			return "Incorrect number of public keys.";
		case wrong_public_key_size:
			return "Invalid public key size.";
		case temporary_listening_error:
			return "Temporary listening error.";
		case cannot_connect_to_broadcast:
			return "Failed to reach to broadcast address.";
		case cannot_connect_to_sender:
			return "Failed to connect to sender.";
		case challenge_connection_broken:
			return "Challenge connection interrupted.";
		case invalid_event_message:
			return "Invalid event message type.";
		case invalid_challenge_request:
			return "Invalid challenge request.";
		case invalid_challenge_response:
			return "Invalid challenge response.";
		case internal_listening_error:
			return "Internal listening error.";
		case internal_connecting_error:
			return "Internal connection error.";
		case internal_sending_error:
			return "Internal sending error.";
		case internal_broadcast_sending_error:
			return "Broadcast sending error.";
		case internal_receiving_error:
			return "Internal receiving error.";
		case internal_broadcast_receiving_error:
			return "Broadcast receiving error.";
		case internal_signing_error:
			return "Error during signing.";
		case internal_verification_error:
			return "Error during verification.";
		case ran_out_of_tries:
			return "Maximum number of retries reached.";
		default:
			return "Unknown error.";
		}
	}
};

}

inline network_event_handler_error::category& network_event_category() {
	static network_event_handler_error::category cat;
	return cat;
}

namespace network_event_handler_error {
	inline std::error_code make_error_code(codes e) {
		return { static_cast<int>(e), network_event_category() };
	}
}

template <>
struct std::is_error_code_enum<network_event_handler_error::codes> : public std::true_type {};

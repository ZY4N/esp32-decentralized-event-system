#include "tasks/test_task.hpp"

#include "crypto/signature_engine.hpp"
#include "keys/event_keys.hpp"
#include "util/feed_the_beast.hpp"
#include "esp_log.h"

std::error_code test_task() {
	std::error_code error;

	if ((error = test_signature_engine())) {
		return error;
	}

	return {};
}


std::error_code test_signature_engine() {

	static constexpr auto TAG = "test_signature_engine";

	std::error_code error;

	feed_the_beast();
	signature_engine signator;
	if ((error = signator.init(private_key))) {
		return error;
	}

	const auto message_view = std::string_view{ "Hallo das ist ein Test!!!" };
	const auto message_span = std::span{ reinterpret_cast<const ztu::u8*>(message_view.data()), message_view.length() };

	std::array<ztu::u8, signature_engine::signature_size> signature;

	feed_the_beast();
	if ((error = signator.sign(message_span, signature))) {
		return error;
	}

	feed_the_beast();
	if ((error = signator.set_verification_key(public_keys[CONFIG_SENDER_ID]))) {
		return error;
	}


	feed_the_beast();
	bool correct = false;
	if ((error = signator.verify(message_span, signature, correct))) {
		return error;
	}

	if (not correct) {
		ESP_LOGI(TAG, "Success!!!");		
	} else {
		ESP_LOGE(TAG, "Signature wrong!!!");
	}

	return {};
}

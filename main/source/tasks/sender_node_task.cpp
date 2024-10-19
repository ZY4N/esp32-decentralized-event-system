#include "tasks/sender_node_task.hpp"

#include "esp_log.h"
#include "wifi/wifi_client_handler.hpp"
#include "networking/network_event_handler.hpp"
#include "events/simple_event.hpp"

#include "keys/event_keys.hpp"

std::error_code sender_node_task() {

	static constexpr auto TAG = "sender_node_task";

	ESP_LOGI(TAG, "in the task!!!");

	std::error_code error;

	wifi::client_handler wifi_client;
	if ((error = wifi_client.connect(
		CONFIG_AP_SSID, CONFIG_AP_PASSWORD,
		//0, 0, 0,
		10, 5'000
	))) {
		ESP_LOGI(TAG, "Got an error!!!");
		return error;
	} else {
		ESP_LOGI(TAG, "Wifi connected.");
	}

	network_event_handler<simple_event, public_keys.size()> event_handler;

	// Important: only init/use after enabling wifi for cryptographic entropy source
	if ((error = event_handler.init(private_key, public_keys, CONFIG_SENDER_ID))) {
		return error;
	} else {
		ESP_LOGI(TAG, "Event handler initialized.");
	}

	if ((error = event_handler.send(simple_event::first_event, 1))) {
		return error;
	} else {
		ESP_LOGI(TAG, "'first_event' sent.");
	}

	const auto events = std::array{ simple_event::second_event };
	simple_event received_event;

	if ((error = event_handler.await(events, received_event, 10))) {
		return error;
	} else {
		ESP_LOGI(TAG, "'second_event' received.");
	}

	ESP_LOGI(TAG, "Success!!");

	return {};
}

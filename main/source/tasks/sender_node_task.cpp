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

	while (true) {
		for (const auto event : {
			simple_event::first_event,
			simple_event::second_event,
			simple_event::three_event,
			simple_event::four_event,
			simple_event::five_event,
			simple_event::six_event,
			simple_event::seven_event,
			simple_event::eight_event,
			simple_event::nine_event,
			simple_event::ten_event
		}) {
			
			const auto event_number = static_cast<int>(event) + 1;
			ESP_LOGI(TAG, "Sending event %d.", event_number);
			if ((error = event_handler.send(event, 1))) {
				ESP_LOGE(TAG, "Error while sending event %d: [%s] %s", event_number, error.category().name(), error.message().c_str());
			} else {
				ESP_LOGI(TAG, "Sent event %d.", event_number);
			}
			vTaskDelay((310 / portTICK_PERIOD_MS) * 2 * event_number);
		}
		ESP_LOGI(TAG, "All events sent!!!");
	}


	return {};
}

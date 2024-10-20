#include "tasks/receiver_node_task.hpp"

#include "esp_log.h"
#include "wifi/wifi_client_handler.hpp"
#include "networking/network_event_handler.hpp"
#include "events/simple_event.hpp"

#include "keys/event_keys.hpp"
#include "driver/gpio.h"


std::error_code receiver_node_task() {

	static constexpr auto TAG = "receiver_node_task";
	static constexpr auto led_pin = GPIO_NUM_2;

	gpio_set_direction(led_pin, GPIO_MODE_OUTPUT);
	gpio_set_level(led_pin, 0);

	std::error_code error;

	wifi::client_handler wifi_client;
	if ((error = wifi_client.connect(
		CONFIG_AP_SSID, CONFIG_AP_PASSWORD,
		10, 5'000
	))) {
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
		
	const auto events = std::array{
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
	};
	simple_event received_event;

	while (true) {
		if ((error = event_handler.await(events, received_event))) {
			ESP_LOGE(TAG, "Error while receiving: [%s] %s", error.category().name(), error.message().c_str());
		} else {
			const auto event_number = static_cast<int>(received_event) + 1;
			ESP_LOGI(TAG, "Received event %d.", event_number);

			for (int i{}; i != event_number; ++i) {
				gpio_set_level(led_pin, 1);
				vTaskDelay(300 / portTICK_PERIOD_MS);
				gpio_set_level(led_pin, 0);
				vTaskDelay(300 / portTICK_PERIOD_MS);
			} 
		}
	}

	return {};
}

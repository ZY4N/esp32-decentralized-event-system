#include "tasks/wifi_ap_task.hpp"

#include "wifi/wifi_access_point_handler.hpp"
#include "keys/event_keys.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


std::error_code wifi_ap_task() {

	wifi::access_point_handler wifi_ap;

	if (auto error = wifi_ap.create(
			CONFIG_AP_SSID,
			CONFIG_AP_PASSWORD,
			1, public_keys.size(),
			10'000
		); error
	) {
		return error;
	}

	while (true) {
		vTaskDelay(10'000 / portTICK_PERIOD_MS);
	}

	return {};
}

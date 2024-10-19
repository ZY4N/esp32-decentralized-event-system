
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tasks/test_task.hpp"
#include "tasks/wifi_ap_task.hpp"
#include "tasks/sender_node_task.hpp"
#include "tasks/receiver_node_task.hpp"


void main_task(void*) {
	while (true) {
		const auto run_task = [](const char* name, auto&& f) {
			ESP_LOGI(name, "Starting task.");
			if (auto e = f(); e) {
				ESP_LOGE(name, "[%s]: %s", e.category().name(), e.message().c_str());
			} else {
				ESP_LOGI(name, "Task exited successfully!");
			}
		};

#ifdef CONFIG_TEST_TASK
		run_task("test_task", test_task);
#endif
#ifdef CONFIG_WIFI_AP_TASK
		run_task("wifi_ap_task", wifi_ap_task);
#endif
#ifdef CONFIG_SENDER_NODE_TASK
		run_task("sender_node_task", sender_node_task);
#endif
#ifdef CONFIG_RECEIVER_NODE_TASK
		run_task("receiver_node_task", receiver_node_task);
#endif

	}
}

extern "C" {

void app_main(void) {
	xTaskCreate(main_task, "Main", 8192, NULL, tskIDLE_PRIORITY + 1, NULL);
}

}

#include "wifi/wifi_client_handler.hpp"
#include "util/esp_error.hpp"
#include "util/safe_strcpy.hpp"
#include "freertos/task.h"
#include "esp_log.h"

#include "util/feed_the_beast.hpp"

static constexpr auto TAG = "client_handler";


std::error_code wifi::client_handler::connect(
	std::string_view ssid, std::string_view password,
	u32 num_retries, u32 timeout_ms
) {
	return connect(
		ssid, password,
		0, 0, 0,
		num_retries, timeout_ms
	);
}

std::error_code wifi::client_handler::connect(
	std::string_view ssid, std::string_view password,
	ztu::u32 ip, ztu::u32 netmask, ztu::u32 gateway,
	ztu::u32 num_retries, ztu::u32 timeout_ms
) {

	using enum esp_error::codes;
	using esp_error::make_error_code;
	
	esp_err_t code;
	std::atomic_flag connected = ATOMIC_FLAG_INIT;
	esp_event_handler_instance_t instance_got_ip{ nullptr }, instance_any_id{ nullptr };

	{
		if ((code = generic_init()) != ESP_OK)
			goto on_error;

		if ((m_handle = esp_netif_create_default_wifi_sta()) == nullptr) {
			code = static_cast<int>(FAIL);
			goto on_error;
		}

		feed_the_beast();
		const wifi_init_config_t default_wifi_config = WIFI_INIT_CONFIG_DEFAULT();
		if ((code = esp_wifi_init(&default_wifi_config)) != ESP_OK) {
			goto on_error;
		}

		connection_state conn_state{
			.ip{ ip }, .netmask{ netmask }, .gateway{ gateway },
			.current_retries{ 0 }, .max_retries{ num_retries },
			.handle{ m_handle }, .connected{ connected }
		};

		auto invoke_event_handler = [](
			void *arg,
			esp_event_base_t event_base,
			i32 event_id,
			void* event_data
		) {
			reinterpret_cast<connection_state*>(arg)->event_handler(
				event_base, event_id, event_data
			);
		};

		if ((code = esp_event_handler_instance_register(
			WIFI_EVENT, ESP_EVENT_ANY_ID, +invoke_event_handler, &conn_state, &instance_got_ip
		)) != ESP_OK) goto on_error;

		if ((code = esp_event_handler_instance_register(
			IP_EVENT, IP_EVENT_STA_GOT_IP, +invoke_event_handler, &conn_state, &instance_any_id
		)) != ESP_OK) goto on_error;

		auto wifi_config = wifi_config_t{
			.sta = {
				.ssid = {},
				.password = {},
				.scan_method {},
				.bssid_set = {},
				.bssid = {},
				.channel = {},
				.listen_interval = {},
				.sort_method = {},
				.threshold = {
					.rssi = {},
					.authmode = WIFI_AUTH_WPA2_PSK
				},
				.pmf_cfg = {
					.capable = true,
					.required = false
				},
				.rm_enabled = {},
				.btm_enabled = {},
				.mbo_enabled = {},
				.reserved = {}
			}
		};

		if (not safe_strcpy(ssid, wifi_config.sta.ssid)) {
			code = static_cast<int>(ERR_WIFI_SSID);
			goto on_error;
		}
		
		if (not safe_strcpy(password, wifi_config.sta.password)) {
			code = static_cast<int>(ERR_WIFI_PASSWORD);
			goto on_error;
		}

		if ((code = esp_wifi_set_mode(WIFI_MODE_STA)) != ESP_OK)
			goto on_error;

		if ((code = esp_wifi_set_config(WIFI_IF_STA, &wifi_config)) != ESP_OK)
			goto on_error;

		if ((code = esp_wifi_start()) != ESP_OK)
			goto on_error;

		// Wait for fixed interval or until connected
		if (timeout_ms) {
			vTaskDelay(timeout_ms / portTICK_PERIOD_MS);
		} else {
			connected.wait(false);
		}

		// Remove event listeners before deleting group
		if ((code = esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip)) != ESP_OK)
			goto on_error;

		if ((code = esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id)) != ESP_OK)
			goto on_error;
	}


	if (connected.test()) {
		return make_error_code(OK);
	} else {
		code = static_cast<int>(ERR_WIFI_NOT_CONNECT);
	}

on_error:

	if (m_handle) {
		esp_netif_destroy_default_wifi(m_handle);
		m_handle = nullptr;
	}

	if (instance_got_ip)
		esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);

	if (instance_any_id)
		esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_any_id);

	return make_error_code(static_cast<esp_error::codes>(code));
}


void wifi::client_handler::disconnect() {
	generic_deinit();
}

wifi::client_handler::~client_handler() {
	disconnect();
}
void wifi::client_handler::connection_state::event_handler(esp_event_base_t event_base, ztu::i32 event_id, void* event_data) {
	ESP_LOGI(TAG, "event_handler: %s %ld", event_base, event_id);
	if (event_base == WIFI_EVENT) {
		switch (event_id) {
			case WIFI_EVENT_STA_START: {
				esp_wifi_connect();
				break;
			}
			case WIFI_EVENT_STA_CONNECTED: {
				if (ip == 0 && netmask == 0 && gateway == 0) {
					ESP_LOGI(TAG, "Waiting for DHCP to assign ip.");
				} else {
					ESP_LOGI(TAG, "Trying to set static ip.");

					auto errc = esp_netif_dhcpc_stop(handle);
					if (errc != ESP_OK and errc != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
						ESP_LOGE(TAG, "Failed to stop dhcp client: %s", esp_error::category().message(errc).c_str());
						break;
					}

					const auto ip_info = esp_netif_ip_info_t{
						.ip{		.addr{ ip		} },
						.netmask{	.addr{ netmask	} },
						.gw{		.addr{ gateway	} }
					};
					if ((errc = esp_netif_set_ip_info(handle, &ip_info)) != ESP_OK) {
						ESP_LOGE(TAG, "Failed to set ip info: %s", esp_error::category().message(errc).c_str());
						break;
					}
					
					ESP_LOGI(TAG, "Set static ip successfully.");
				}
				break;
			}
			case WIFI_EVENT_STA_DISCONNECTED: {
				if (current_retries < max_retries) {
					esp_wifi_connect();
					current_retries++;
					ESP_LOGI(TAG, "retry connecting to the AP");
				} else {
					connected.clear();
					connected.notify_one();
				}
				ESP_LOGI(TAG,"connect to the AP fail");
				break;
			}
		}
	} else if (event_base == IP_EVENT) {
		auto event = reinterpret_cast<ip_event_got_ip_t*>(event_data);
        ESP_LOGI(TAG, "received ip:" IPSTR, IP2STR(&event->ip_info.ip));
        current_retries = 0;
		connected.test_and_set();
		connected.notify_one();
	}
}

#include <freertos/FreeRTOS.h>
#include "freertos/task.h"

inline void feed_the_beast() {
	vTaskDelay(1);
}

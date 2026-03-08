#pragma once

#include "esp_err.h"
#include "freertos/queue.h"

esp_err_t input_init(QueueHandle_t app_event_queue);

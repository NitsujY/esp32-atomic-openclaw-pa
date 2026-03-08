#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

esp_err_t audio_capture_init(void);
esp_err_t audio_capture_start(void);
esp_err_t audio_capture_stop(void);
size_t audio_capture_drain(uint8_t *buffer, size_t max_bytes, TickType_t timeout);
size_t audio_capture_high_watermark(void);

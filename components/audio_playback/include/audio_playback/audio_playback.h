#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/queue.h"

typedef struct {
    const uint8_t *data;
    size_t length;
    bool is_wav;
} audio_playback_buffer_t;

esp_err_t audio_playback_init(QueueHandle_t app_event_queue);
esp_err_t audio_playback_enqueue(const audio_playback_buffer_t *buffer);

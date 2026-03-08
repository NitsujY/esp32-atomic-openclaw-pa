#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_state/app_state.h"
#include "audio_transport/audio_transport.h"
#include "audio_playback/audio_playback.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef audio_playback_buffer_t bridge_client_audio_t;

esp_err_t bridge_client_init(QueueHandle_t app_event_queue);
esp_err_t bridge_client_start(void);
esp_err_t bridge_client_submit_utterance(app_mode_t mode, const audio_transport_utterance_t *utterance);
esp_err_t bridge_client_get_audio(bridge_client_audio_t *audio);

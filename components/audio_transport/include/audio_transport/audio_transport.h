#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    const uint8_t *data;
    size_t length;
    bool is_wav;
} audio_transport_utterance_t;

esp_err_t audio_transport_init(void);
void audio_transport_reset(void);
esp_err_t audio_transport_finalize_from_capture(audio_transport_utterance_t *utterance);
size_t audio_transport_high_watermark(void);

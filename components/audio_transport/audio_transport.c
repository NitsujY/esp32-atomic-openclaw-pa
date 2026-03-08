#include "audio_transport/audio_transport.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "audio_capture/audio_capture.h"
#include "config/app_config.h"

static uint8_t s_transport_buffer[APP_CONFIG_MAX_UPLOAD_BYTES];
static size_t s_transport_length;
static size_t s_transport_high_watermark;

static void write_le16(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t) (value & 0xffU);
    buffer[1] = (uint8_t) ((value >> 8) & 0xffU);
}

static void write_le32(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t) (value & 0xffU);
    buffer[1] = (uint8_t) ((value >> 8) & 0xffU);
    buffer[2] = (uint8_t) ((value >> 16) & 0xffU);
    buffer[3] = (uint8_t) ((value >> 24) & 0xffU);
}

static void prepend_wav_header(size_t pcm_length)
{
    memmove(s_transport_buffer + 44, s_transport_buffer, pcm_length);
    memcpy(s_transport_buffer, "RIFF", 4);
    write_le32(s_transport_buffer + 4, (uint32_t) (pcm_length + 36));
    memcpy(s_transport_buffer + 8, "WAVEfmt ", 8);
    write_le32(s_transport_buffer + 16, 16);
    write_le16(s_transport_buffer + 20, 1);
    write_le16(s_transport_buffer + 22, 1);
    write_le32(s_transport_buffer + 24, APP_CONFIG_SAMPLE_RATE);
    write_le32(s_transport_buffer + 28, APP_CONFIG_SAMPLE_RATE * APP_CONFIG_PCM_FRAME_BYTES);
    write_le16(s_transport_buffer + 32, APP_CONFIG_PCM_FRAME_BYTES);
    write_le16(s_transport_buffer + 34, 16);
    memcpy(s_transport_buffer + 36, "data", 4);
    write_le32(s_transport_buffer + 40, (uint32_t) pcm_length);
    s_transport_length = pcm_length + 44;
}

esp_err_t audio_transport_init(void)
{
    audio_transport_reset();
    return ESP_OK;
}

void audio_transport_reset(void)
{
    s_transport_length = 0;
}

esp_err_t audio_transport_finalize_from_capture(audio_transport_utterance_t *utterance)
{
    size_t pcm_length = audio_capture_drain(s_transport_buffer, APP_CONFIG_MAX_PCM_BYTES,
        pdMS_TO_TICKS(100));

    if (pcm_length > s_transport_high_watermark) {
        s_transport_high_watermark = pcm_length;
    }

    s_transport_length = pcm_length;
    if (app_config_wrap_wav() && pcm_length > 0U) {
        prepend_wav_header(pcm_length);
    }

    utterance->data = s_transport_buffer;
    utterance->length = s_transport_length;
    utterance->is_wav = app_config_wrap_wav();

    return ESP_OK;
}

size_t audio_transport_high_watermark(void)
{
    return s_transport_high_watermark;
}

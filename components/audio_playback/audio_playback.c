#include "audio_playback/audio_playback.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_state/app_state.h"
#include "config/app_config.h"
#include "driver/i2s.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "audio_playback";

static QueueHandle_t s_playback_queue;
static QueueHandle_t s_app_event_queue;
static bool s_i2s_ready;

static const uint8_t *skip_wav_header(const audio_playback_buffer_t *buffer, size_t *length)
{
    if (buffer->is_wav && buffer->length > 44U) {
        *length = buffer->length - 44U;
        return buffer->data + 44;
    }

    *length = buffer->length;
    return buffer->data;
}

static esp_err_t maybe_init_i2s(void)
{
    i2s_config_t config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = APP_CONFIG_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0,
    };
    i2s_pin_config_t pins = {
        .bck_io_num = CONFIG_OPENCLAW_PIN_I2S_SPK_BCK,
        .ws_io_num = CONFIG_OPENCLAW_PIN_I2S_SPK_WS,
        .data_out_num = CONFIG_OPENCLAW_PIN_I2S_SPK_DATA,
        .data_in_num = I2S_PIN_NO_CHANGE,
    };

    if (CONFIG_OPENCLAW_PIN_I2S_SPK_BCK < 0 || CONFIG_OPENCLAW_PIN_I2S_SPK_WS < 0 ||
        CONFIG_OPENCLAW_PIN_I2S_SPK_DATA < 0) {
        ESP_LOGW(TAG, "Speaker pins are unset, playback will stay in stub mode");
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(i2s_driver_install(I2S_NUM_1, &config, 0, NULL), TAG,
        "i2s_driver_install failed");
    ESP_RETURN_ON_ERROR(i2s_set_pin(I2S_NUM_1, &pins), TAG, "i2s_set_pin failed");
    ESP_RETURN_ON_ERROR(i2s_zero_dma_buffer(I2S_NUM_1), TAG, "i2s_zero_dma_buffer failed");

    s_i2s_ready = true;
    return ESP_OK;
}

static void playback_task(void *arg)
{
    audio_playback_buffer_t buffer = {0};
    app_event_t complete_event = {.type = APP_EVENT_PLAYBACK_DONE};

    (void) arg;

    while (xQueueReceive(s_playback_queue, &buffer, portMAX_DELAY) == pdTRUE) {
        size_t payload_length = 0;
        const uint8_t *payload = skip_wav_header(&buffer, &payload_length);

        if (s_i2s_ready) {
            size_t bytes_written = 0;
            i2s_write(I2S_NUM_1, payload, payload_length, &bytes_written, portMAX_DELAY);
        } else {
            vTaskDelay(pdMS_TO_TICKS((payload_length / APP_CONFIG_PCM_FRAME_BYTES * 1000) /
                APP_CONFIG_SAMPLE_RATE));
        }

        xQueueSend(s_app_event_queue, &complete_event, 0);
    }
}

esp_err_t audio_playback_init(QueueHandle_t app_event_queue)
{
    s_app_event_queue = app_event_queue;
    s_playback_queue = xQueueCreate(4, sizeof(audio_playback_buffer_t));
    if (s_playback_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(maybe_init_i2s(), TAG, "maybe_init_i2s failed");
    xTaskCreate(playback_task, "playback_task", 4096, NULL, 5, NULL);
    return ESP_OK;
}

esp_err_t audio_playback_enqueue(const audio_playback_buffer_t *buffer)
{
    return xQueueSend(s_playback_queue, buffer, pdMS_TO_TICKS(50)) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

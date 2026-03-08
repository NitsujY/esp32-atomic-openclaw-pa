#include "audio_capture/audio_capture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "config/app_config.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"

static const char *TAG = "audio_capture";

static RingbufHandle_t s_ringbuffer;
static bool s_capture_active;
static bool s_i2s_ready;
static size_t s_high_watermark;

static void capture_task(void *arg)
{
    uint8_t dma_buffer[512];
    size_t bytes_read = 0;

    (void) arg;

    while (true) {
        if (!s_capture_active || !s_i2s_ready) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (i2s_read(I2S_NUM_0, dma_buffer, sizeof(dma_buffer), &bytes_read, pdMS_TO_TICKS(50)) != ESP_OK) {
            continue;
        }

        if (bytes_read == 0U) {
            continue;
        }

        if (xRingbufferSend(s_ringbuffer, dma_buffer, bytes_read, 0) != pdTRUE) {
            ESP_LOGW(TAG, "Capture ringbuffer full, dropping %u bytes", (unsigned) bytes_read);
            continue;
        }

        if (xRingbufferGetCurFreeSize(s_ringbuffer) < (APP_CONFIG_MAX_PCM_BYTES - s_high_watermark)) {
            s_high_watermark = APP_CONFIG_MAX_PCM_BYTES - xRingbufferGetCurFreeSize(s_ringbuffer);
        }
    }
}

static esp_err_t maybe_init_i2s(void)
{
    i2s_config_t config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = APP_CONFIG_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0,
    };
    i2s_pin_config_t pins = {
        .bck_io_num = CONFIG_OPENCLAW_PIN_I2S_MIC_BCK,
        .ws_io_num = CONFIG_OPENCLAW_PIN_I2S_MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = CONFIG_OPENCLAW_PIN_I2S_MIC_DATA,
    };

    if (CONFIG_OPENCLAW_PIN_I2S_MIC_BCK < 0 || CONFIG_OPENCLAW_PIN_I2S_MIC_WS < 0 ||
        CONFIG_OPENCLAW_PIN_I2S_MIC_DATA < 0) {
        ESP_LOGW(TAG, "Mic pins are unset, capture will stay in stub mode");
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(i2s_driver_install(I2S_NUM_0, &config, 0, NULL), TAG,
        "i2s_driver_install failed");
    ESP_RETURN_ON_ERROR(i2s_set_pin(I2S_NUM_0, &pins), TAG, "i2s_set_pin failed");
    ESP_RETURN_ON_ERROR(i2s_zero_dma_buffer(I2S_NUM_0), TAG, "i2s_zero_dma_buffer failed");

    s_i2s_ready = true;
    return ESP_OK;
}

esp_err_t audio_capture_init(void)
{
    s_ringbuffer = xRingbufferCreate(APP_CONFIG_MAX_PCM_BYTES, RINGBUF_TYPE_BYTEBUF);
    if (s_ringbuffer == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(maybe_init_i2s(), TAG, "maybe_init_i2s failed");
    xTaskCreate(capture_task, "capture_task", 4096, NULL, 5, NULL);
    return ESP_OK;
}

esp_err_t audio_capture_start(void)
{
    s_capture_active = true;
    return ESP_OK;
}

esp_err_t audio_capture_stop(void)
{
    s_capture_active = false;
    return ESP_OK;
}

size_t audio_capture_drain(uint8_t *buffer, size_t max_bytes, TickType_t timeout)
{
    size_t total = 0;

    while (total < max_bytes) {
        size_t item_size = 0;
        uint8_t *item = xRingbufferReceiveUpTo(s_ringbuffer, &item_size, timeout, max_bytes - total);
        if (item == NULL) {
            break;
        }

        memcpy(buffer + total, item, item_size);
        total += item_size;
        vRingbufferReturnItem(s_ringbuffer, item);
        timeout = 0;
    }

    return total;
}

size_t audio_capture_high_watermark(void)
{
    return s_high_watermark;
}

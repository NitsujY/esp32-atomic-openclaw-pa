#include "bridge_client/bridge_client.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "config/app_config.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

typedef struct {
    app_mode_t mode;
    size_t length;
    bool is_wav;
    uint8_t payload[APP_CONFIG_MAX_UPLOAD_BYTES];
} bridge_request_t;

static const char *TAG = "bridge_client";

static QueueHandle_t s_app_event_queue;
static QueueHandle_t s_request_queue;
static bridge_client_audio_t s_last_audio;
static uint8_t s_last_audio_buffer[APP_CONFIG_MAX_UPLOAD_BYTES];

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

static void push_app_event(app_event_type_t type, esp_err_t error_code)
{
    app_event_t event = {
        .type = type,
        .error_code = error_code,
    };

    xQueueSend(s_app_event_queue, &event, 0);
}

static void synthesize_stub_response(void)
{
    const uint32_t wave_period = APP_CONFIG_SAMPLE_RATE / 440U;
    const size_t sample_count = APP_CONFIG_SAMPLE_RATE / 2;
    const size_t pcm_length = sample_count * sizeof(int16_t);
    int16_t *samples = (int16_t *) (s_last_audio_buffer + 44);

    for (size_t index = 0; index < sample_count; ++index) {
        uint32_t position = (uint32_t) (index % wave_period);
        int32_t centered = (int32_t) ((position * 16000U) / wave_period) - 8000;

        samples[index] = (int16_t) centered;
    }

    memcpy(s_last_audio_buffer, "RIFF", 4);
    write_le32(s_last_audio_buffer + 4, (uint32_t) (pcm_length + 36));
    memcpy(s_last_audio_buffer + 8, "WAVEfmt ", 8);
    write_le32(s_last_audio_buffer + 16, 16);
    write_le16(s_last_audio_buffer + 20, 1);
    write_le16(s_last_audio_buffer + 22, 1);
    write_le32(s_last_audio_buffer + 24, APP_CONFIG_SAMPLE_RATE);
    write_le32(s_last_audio_buffer + 28, APP_CONFIG_SAMPLE_RATE * 2U);
    write_le16(s_last_audio_buffer + 32, 2);
    write_le16(s_last_audio_buffer + 34, 16);
    memcpy(s_last_audio_buffer + 36, "data", 4);
    write_le32(s_last_audio_buffer + 40, (uint32_t) pcm_length);

    s_last_audio.data = s_last_audio_buffer;
    s_last_audio.length = pcm_length + 44;
    s_last_audio.is_wav = true;
}

static void worker_task(void *arg)
{
    bridge_request_t request = {0};

    (void) arg;

    while (xQueueReceive(s_request_queue, &request, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "bridge request mode=%s bytes=%u transport=%s", app_mode_name(request.mode),
            (unsigned) request.length, app_config_bridge_stub_enabled() ? "stub" : "http-todo");
        push_app_event(APP_EVENT_UPLOAD_STARTED, ESP_OK);
        vTaskDelay(pdMS_TO_TICKS(150));
        push_app_event(APP_EVENT_UPLOAD_DONE, ESP_OK);
        vTaskDelay(pdMS_TO_TICKS(600));

        if (!app_config_bridge_stub_enabled()) {
            push_app_event(APP_EVENT_ERROR, ESP_ERR_NOT_SUPPORTED);
            continue;
        }

        synthesize_stub_response();
        push_app_event(APP_EVENT_RESPONSE_READY, ESP_OK);
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void) arg;
    (void) event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        push_app_event(APP_EVENT_WIFI_DISCONNECTED, ESP_ERR_WIFI_NOT_CONNECT);
        esp_wifi_connect();
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        push_app_event(APP_EVENT_WIFI_CONNECTED, ESP_OK);
    }
}

esp_err_t bridge_client_init(QueueHandle_t app_event_queue)
{
    s_app_event_queue = app_event_queue;
    s_request_queue = xQueueCreate(2, sizeof(bridge_request_t));
    if (s_request_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "esp_netif_init failed");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "esp_event_loop_create_default failed");
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_config), TAG, "esp_wifi_init failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL),
        TAG, "event handler register failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL),
        TAG, "IP event handler register failed");

    xTaskCreate(worker_task, "bridge_worker", 4096, NULL, 4, NULL);
    return ESP_OK;
}

esp_err_t bridge_client_start(void)
{
    wifi_config_t wifi_config = {0};

    strlcpy((char *) wifi_config.sta.ssid, app_config_wifi_ssid(), sizeof(wifi_config.sta.ssid));
    strlcpy((char *) wifi_config.sta.password, app_config_wifi_password(), sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "esp_wifi_set_mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "esp_wifi_set_config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "esp_wifi_start failed");
    return ESP_OK;
}

esp_err_t bridge_client_submit_utterance(app_mode_t mode, const audio_transport_utterance_t *utterance)
{
    bridge_request_t request = {
        .mode = mode,
        .length = utterance->length,
        .is_wav = utterance->is_wav,
    };

    if (utterance->length > sizeof(request.payload)) {
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(request.payload, utterance->data, utterance->length);
    return xQueueSend(s_request_queue, &request, pdMS_TO_TICKS(50)) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t bridge_client_get_audio(bridge_client_audio_t *audio)
{
    if (s_last_audio.length == 0U) {
        return ESP_ERR_NOT_FOUND;
    }

    *audio = s_last_audio;
    return ESP_OK;
}
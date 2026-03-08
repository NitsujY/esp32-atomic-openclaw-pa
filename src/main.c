#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include "audio_capture/audio_capture.h"
#include "audio_playback/audio_playback.h"
#include "audio_transport/audio_transport.h"
#include "bridge_client/bridge_client.h"
#include "config/app_config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "input/input.h"
#include "nvs_flash.h"
#include "ui/ui.h"

static const char *TAG = "openclaw_main";

static void render_state(const app_state_context_t *app)
{
    app_state_snapshot_t snapshot = app_state_snapshot(app);

    ui_render(&snapshot);
}

static void handle_audio_capture(void)
{
    audio_transport_reset();
    ESP_ERROR_CHECK_WITHOUT_ABORT(audio_capture_start());
}

static void handle_utterance_submit(const app_state_context_t *app, uint32_t duration_ms)
{
    audio_transport_utterance_t utterance = {0};

    if (duration_ms < APP_CONFIG_PTT_MIN_DURATION_MS) {
        ESP_LOGW(TAG, "Ignoring short press: %" PRIu32 " ms", duration_ms);
        return;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(audio_capture_stop());
    ESP_ERROR_CHECK_WITHOUT_ABORT(audio_transport_finalize_from_capture(&utterance));

    if (utterance.length == 0U) {
        ESP_LOGW(TAG, "No audio captured");
        return;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(bridge_client_submit_utterance(app_state_mode(app), &utterance));
}

void app_main(void)
{
    QueueHandle_t app_event_queue = NULL;
    app_state_context_t app = {0};
    app_event_t event = {0};
    app_state_snapshot_t before = {0};
    esp_err_t err = ESP_OK;

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    app_event_queue = xQueueCreate(16, sizeof(app_event_t));
    configASSERT(app_event_queue != NULL);

    app_state_init(&app);

    ESP_ERROR_CHECK(ui_init());
    render_state(&app);

    ESP_ERROR_CHECK(audio_capture_init());
    ESP_ERROR_CHECK(audio_transport_init());
    ESP_ERROR_CHECK(audio_playback_init(app_event_queue));
    ESP_ERROR_CHECK(input_init(app_event_queue));
    ESP_ERROR_CHECK(bridge_client_init(app_event_queue));
    ESP_ERROR_CHECK(bridge_client_start());

    while (xQueueReceive(app_event_queue, &event, portMAX_DELAY) == pdTRUE) {
        app_state_snapshot_t after = {0};

        before = app_state_snapshot(&app);

        ESP_LOGI(TAG, "event=%s state=%s mode=%s", app_event_type_name(event.type),
            app_state_name(before.state), app_mode_name(before.mode));

        app_state_handle_event(&app, &event);

        switch (event.type) {
        case APP_EVENT_PTT_PRESSED:
            handle_audio_capture();
            break;
        case APP_EVENT_PTT_RELEASED:
            handle_utterance_submit(&app, event.value_u32);
            break;
        case APP_EVENT_RESPONSE_READY: {
            bridge_client_audio_t response = {0};

            if (bridge_client_get_audio(&response) == ESP_OK && response.length > 0U) {
                app_event_t playback_started = {.type = APP_EVENT_PLAYBACK_STARTED};
                app_state_handle_event(&app, &playback_started);
                render_state(&app);
                ESP_ERROR_CHECK_WITHOUT_ABORT(audio_playback_enqueue(&response));
            }
            break;
        }
        default:
            break;
        }

        after = app_state_snapshot(&app);
        if (memcmp(&before, &after, sizeof(before)) != 0) {
            render_state(&app);
        }
    }
}

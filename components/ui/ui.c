#include "ui/ui.h"

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "ui";

esp_err_t ui_init(void)
{
    ESP_LOGI(TAG, "UI initialized in log-only mode until LCD module and pinout are confirmed");
    return ESP_OK;
}

void ui_render(const app_state_snapshot_t *snapshot)
{
    const char *headline = "";

    switch (snapshot->state) {
    case APP_STATE_IDLE:
        headline = snapshot->mode == APP_MODE_WORK ? "WORK idle" : "KID idle";
        break;
    case APP_STATE_LISTENING:
        headline = "Listening";
        break;
    case APP_STATE_UPLOADING:
        headline = "Uploading";
        break;
    case APP_STATE_WAITING_RESPONSE:
        headline = "Thinking";
        break;
    case APP_STATE_PLAYING:
        headline = "Playing";
        break;
    case APP_STATE_ERROR:
        headline = "Recoverable error";
        break;
    default:
        headline = "Unknown";
        break;
    }

    ESP_LOGI(TAG, "screen='%s' wifi=%s mode=%s error=%s", headline,
        snapshot->wifi_connected ? "connected" : "disconnected", app_mode_name(snapshot->mode),
        esp_err_to_name(snapshot->last_error));
}

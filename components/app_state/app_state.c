#include "app_state/app_state.h"

#include "config/app_config.h"
#include "esp_wifi.h"

void app_state_init(app_state_context_t *context)
{
    context->state = APP_STATE_IDLE;
    context->mode = app_config_default_mode_work() ? APP_MODE_WORK : APP_MODE_KID;
    context->wifi_connected = false;
    context->last_error = ESP_OK;
}

void app_state_handle_event(app_state_context_t *context, const app_event_t *event)
{
    switch (event->type) {
    case APP_EVENT_WIFI_CONNECTED:
        context->wifi_connected = true;
        if (context->state == APP_STATE_ERROR && context->last_error == ESP_ERR_WIFI_NOT_CONNECT) {
            context->state = APP_STATE_IDLE;
            context->last_error = ESP_OK;
        }
        break;
    case APP_EVENT_WIFI_DISCONNECTED:
        context->wifi_connected = false;
        if (context->state == APP_STATE_IDLE) {
            break;
        }
        context->state = APP_STATE_ERROR;
        context->last_error = ESP_ERR_WIFI_NOT_CONNECT;
        break;
    case APP_EVENT_MODE_TOGGLE:
        context->mode = (context->mode == APP_MODE_WORK) ? APP_MODE_KID : APP_MODE_WORK;
        break;
    case APP_EVENT_PTT_PRESSED:
        if (context->state == APP_STATE_IDLE) {
            context->state = APP_STATE_LISTENING;
            context->last_error = ESP_OK;
        }
        break;
    case APP_EVENT_PTT_RELEASED:
    case APP_EVENT_UPLOAD_STARTED:
        if (context->state == APP_STATE_LISTENING || context->state == APP_STATE_IDLE) {
            context->state = APP_STATE_UPLOADING;
        }
        break;
    case APP_EVENT_UPLOAD_DONE:
        if (context->state == APP_STATE_UPLOADING) {
            context->state = APP_STATE_WAITING_RESPONSE;
        }
        break;
    case APP_EVENT_RESPONSE_READY:
    case APP_EVENT_PLAYBACK_STARTED:
        context->state = APP_STATE_PLAYING;
        break;
    case APP_EVENT_PLAYBACK_DONE:
    case APP_EVENT_CLEAR_ERROR:
        context->state = APP_STATE_IDLE;
        context->last_error = ESP_OK;
        break;
    case APP_EVENT_ERROR:
        context->state = APP_STATE_ERROR;
        context->last_error = event->error_code;
        break;
    case APP_EVENT_BOOT:
    default:
        break;
    }
}

app_state_snapshot_t app_state_snapshot(const app_state_context_t *context)
{
    return *context;
}

app_state_t app_state_current(const app_state_context_t *context)
{
    return context->state;
}

app_mode_t app_state_mode(const app_state_context_t *context)
{
    return context->mode;
}

const char *app_state_name(app_state_t state)
{
    switch (state) {
    case APP_STATE_IDLE:
        return "idle";
    case APP_STATE_LISTENING:
        return "listening";
    case APP_STATE_UPLOADING:
        return "uploading";
    case APP_STATE_WAITING_RESPONSE:
        return "waiting_response";
    case APP_STATE_PLAYING:
        return "playing";
    case APP_STATE_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

const char *app_mode_name(app_mode_t mode)
{
    return mode == APP_MODE_WORK ? "work" : "kid";
}

const char *app_event_type_name(app_event_type_t type)
{
    switch (type) {
    case APP_EVENT_BOOT:
        return "boot";
    case APP_EVENT_WIFI_CONNECTED:
        return "wifi_connected";
    case APP_EVENT_WIFI_DISCONNECTED:
        return "wifi_disconnected";
    case APP_EVENT_MODE_TOGGLE:
        return "mode_toggle";
    case APP_EVENT_PTT_PRESSED:
        return "ptt_pressed";
    case APP_EVENT_PTT_RELEASED:
        return "ptt_released";
    case APP_EVENT_UPLOAD_STARTED:
        return "upload_started";
    case APP_EVENT_UPLOAD_DONE:
        return "upload_done";
    case APP_EVENT_RESPONSE_READY:
        return "response_ready";
    case APP_EVENT_PLAYBACK_STARTED:
        return "playback_started";
    case APP_EVENT_PLAYBACK_DONE:
        return "playback_done";
    case APP_EVENT_ERROR:
        return "error";
    case APP_EVENT_CLEAR_ERROR:
        return "clear_error";
    default:
        return "unknown";
    }
}

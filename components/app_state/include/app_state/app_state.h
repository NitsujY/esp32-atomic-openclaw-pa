#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum {
    APP_MODE_WORK = 0,
    APP_MODE_KID,
} app_mode_t;

typedef enum {
    APP_STATE_IDLE = 0,
    APP_STATE_LISTENING,
    APP_STATE_UPLOADING,
    APP_STATE_WAITING_RESPONSE,
    APP_STATE_PLAYING,
    APP_STATE_ERROR,
} app_state_t;

typedef enum {
    APP_EVENT_BOOT = 0,
    APP_EVENT_WIFI_CONNECTED,
    APP_EVENT_WIFI_DISCONNECTED,
    APP_EVENT_MODE_TOGGLE,
    APP_EVENT_PTT_PRESSED,
    APP_EVENT_PTT_RELEASED,
    APP_EVENT_UPLOAD_STARTED,
    APP_EVENT_UPLOAD_DONE,
    APP_EVENT_RESPONSE_READY,
    APP_EVENT_PLAYBACK_STARTED,
    APP_EVENT_PLAYBACK_DONE,
    APP_EVENT_ERROR,
    APP_EVENT_CLEAR_ERROR,
} app_event_type_t;

typedef struct {
    app_event_type_t type;
    esp_err_t error_code;
    uint32_t value_u32;
} app_event_t;

typedef struct {
    app_state_t state;
    app_mode_t mode;
    bool wifi_connected;
    esp_err_t last_error;
} app_state_context_t;

typedef app_state_context_t app_state_snapshot_t;

void app_state_init(app_state_context_t *context);
void app_state_handle_event(app_state_context_t *context, const app_event_t *event);
app_state_snapshot_t app_state_snapshot(const app_state_context_t *context);
app_state_t app_state_current(const app_state_context_t *context);
app_mode_t app_state_mode(const app_state_context_t *context);

const char *app_state_name(app_state_t state);
const char *app_mode_name(app_mode_t mode);
const char *app_event_type_name(app_event_type_t type);

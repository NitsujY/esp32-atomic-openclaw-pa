#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sdkconfig.h"

#define APP_CONFIG_SAMPLE_RATE CONFIG_OPENCLAW_AUDIO_SAMPLE_RATE
#define APP_CONFIG_PCM_FRAME_BYTES 2
#define APP_CONFIG_MAX_PCM_BYTES ((CONFIG_OPENCLAW_AUDIO_SAMPLE_RATE * APP_CONFIG_PCM_FRAME_BYTES * CONFIG_OPENCLAW_MAX_UTTERANCE_MS) / 1000)
#define APP_CONFIG_MAX_UPLOAD_BYTES (APP_CONFIG_MAX_PCM_BYTES + 44)
#define APP_CONFIG_PTT_MIN_DURATION_MS CONFIG_OPENCLAW_PTT_MIN_DURATION_MS

const char *app_config_default_mode_name(void);

static inline const char *app_config_wifi_ssid(void)
{
    return CONFIG_OPENCLAW_WIFI_SSID;
}

static inline const char *app_config_wifi_password(void)
{
    return CONFIG_OPENCLAW_WIFI_PASSWORD;
}

static inline const char *app_config_bridge_url(void)
{
    return CONFIG_OPENCLAW_BRIDGE_URL;
}

static inline const char *app_config_bridge_api_key(void)
{
    return CONFIG_OPENCLAW_BRIDGE_API_KEY;
}

static inline const char *app_config_device_id(void)
{
    return CONFIG_OPENCLAW_DEVICE_ID;
}

static inline bool app_config_default_mode_work(void)
{
#if CONFIG_OPENCLAW_DEFAULT_MODE_WORK
    return true;
#else
    return false;
#endif
}

static inline bool app_config_wrap_wav(void)
{
#if CONFIG_OPENCLAW_WRAP_WAV
    return true;
#else
    return false;
#endif
}

static inline bool app_config_bridge_stub_enabled(void)
{
#if CONFIG_OPENCLAW_ENABLE_BRIDGE_STUB
    return true;
#else
    return false;
#endif
}

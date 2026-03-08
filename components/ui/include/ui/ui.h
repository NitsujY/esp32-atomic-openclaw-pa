#pragma once

#include "app_state/app_state.h"
#include "esp_err.h"

esp_err_t ui_init(void);
void ui_render(const app_state_snapshot_t *snapshot);

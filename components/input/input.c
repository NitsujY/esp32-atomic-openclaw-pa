#include "input/input.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_state/app_state.h"
#include "config/app_config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

typedef enum {
    BUTTON_SIDE = 0,
    BUTTON_CENTER,
    BUTTON_COUNT,
} button_id_t;

typedef struct {
    button_id_t button;
    int level;
    TickType_t tick;
} raw_button_event_t;

typedef struct {
    int gpio_num;
    TickType_t last_change_tick;
    TickType_t press_tick;
    int stable_level;
    bool enabled;
} button_state_t;

static const char *TAG = "input";

static QueueHandle_t s_app_event_queue;
static QueueHandle_t s_raw_button_queue;
static button_state_t s_buttons[BUTTON_COUNT];

static void IRAM_ATTR button_isr_handler(void *arg)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    raw_button_event_t event = {
        .button = (button_id_t) (uintptr_t) arg,
        .level = gpio_get_level(s_buttons[(uintptr_t) arg].gpio_num),
        .tick = xTaskGetTickCountFromISR(),
    };

    xQueueSendFromISR(s_raw_button_queue, &event, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void publish_event(app_event_type_t type, uint32_t value_u32)
{
    app_event_t event = {
        .type = type,
        .value_u32 = value_u32,
    };

    xQueueSend(s_app_event_queue, &event, 0);
}

static void handle_button_event(const raw_button_event_t *raw)
{
    button_state_t *button = &s_buttons[raw->button];
    TickType_t debounce_ticks = pdMS_TO_TICKS(CONFIG_OPENCLAW_BUTTON_DEBOUNCE_MS);

    if (!button->enabled) {
        return;
    }

    if ((raw->tick - button->last_change_tick) < debounce_ticks) {
        return;
    }

    if (raw->level == button->stable_level) {
        return;
    }

    button->stable_level = raw->level;
    button->last_change_tick = raw->tick;

    if (raw->button == BUTTON_SIDE) {
        if (raw->level == 1) {
            publish_event(APP_EVENT_MODE_TOGGLE, 0);
        }
        return;
    }

    if (raw->level == 0) {
        button->press_tick = raw->tick;
        publish_event(APP_EVENT_PTT_PRESSED, 0);
        return;
    }

    publish_event(APP_EVENT_PTT_RELEASED,
        pdTICKS_TO_MS(raw->tick - button->press_tick));
}

static void input_task(void *arg)
{
    raw_button_event_t raw = {0};

    (void) arg;

    while (xQueueReceive(s_raw_button_queue, &raw, portMAX_DELAY) == pdTRUE) {
        handle_button_event(&raw);
    }
}

static esp_err_t configure_button(button_id_t id, int gpio_num)
{
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << gpio_num,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };

    s_buttons[id].gpio_num = gpio_num;
    s_buttons[id].enabled = gpio_num >= 0;

    if (!s_buttons[id].enabled) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(gpio_config(&config), TAG, "gpio_config failed");
    s_buttons[id].stable_level = gpio_get_level(gpio_num);
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(gpio_num, button_isr_handler, (void *) (uintptr_t) id),
        TAG, "gpio_isr_handler_add failed");

    return ESP_OK;
}

esp_err_t input_init(QueueHandle_t app_event_queue)
{
    esp_err_t err = ESP_OK;

    s_app_event_queue = app_event_queue;
    s_raw_button_queue = xQueueCreate(16, sizeof(raw_button_event_t));
    if (s_raw_button_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_RETURN_ON_ERROR(err, TAG, "gpio_install_isr_service failed");
    }
    ESP_RETURN_ON_ERROR(configure_button(BUTTON_SIDE, CONFIG_OPENCLAW_PIN_BUTTON_SIDE), TAG,
        "configure side button failed");
    ESP_RETURN_ON_ERROR(configure_button(BUTTON_CENTER, CONFIG_OPENCLAW_PIN_BUTTON_CENTER), TAG,
        "configure center button failed");

    xTaskCreate(input_task, "input_task", 4096, NULL, 6, NULL);

    ESP_LOGI(TAG, "Input initialized, side=%d center=%d", CONFIG_OPENCLAW_PIN_BUTTON_SIDE,
        CONFIG_OPENCLAW_PIN_BUTTON_CENTER);

    return ESP_OK;
}

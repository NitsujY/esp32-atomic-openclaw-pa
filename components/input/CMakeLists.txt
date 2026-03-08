idf_component_register(
    SRCS "input.c"
    INCLUDE_DIRS "include"
    REQUIRES app_state config driver esp_timer
)

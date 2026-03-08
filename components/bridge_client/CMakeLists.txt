idf_component_register(
    SRCS "bridge_client.c"
    INCLUDE_DIRS "include"
    REQUIRES app_state audio_playback audio_transport config esp_wifi esp_netif nvs_flash
)

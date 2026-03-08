idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES app_state input ui audio_capture audio_playback audio_transport bridge_client config nvs_flash
)

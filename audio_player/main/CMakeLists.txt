set(component_srcs "main.c")

idf_component_register(
    SRCS "${component_srcs}"
    INCLUDE_DIRS "."
    REQUIRES led_embebido nvs_flash wifi_embebido servidor_embebido mqtt_embebido audio_embebido config_embebido spiffs_embebido queue_embebido tiempo_embebido event_logger
)
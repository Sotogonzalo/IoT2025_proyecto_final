set(component_srcs "src/servidor_embebido.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES mqtt_embebido wifi_embebido config_embebido audio_embebido queue_embebido event_logger
                       REQUIRES esp_http_server spiffs json)
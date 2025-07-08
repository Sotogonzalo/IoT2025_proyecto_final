set(component_srcs "src/wifi_embebido.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_wifi esp_event nvs_flash servidor_embebido mqtt_embebido config_embebido)

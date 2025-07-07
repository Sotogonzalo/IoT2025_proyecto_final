set(component_srcs "src/tiempo_embebido.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_event nvs_flash esp_netif)
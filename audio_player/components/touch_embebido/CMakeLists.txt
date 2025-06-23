set(component_srcs "src/touch_embebido.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver" "esp_rom_delay_ms" "led_embebido"
                       REQUIRES "")

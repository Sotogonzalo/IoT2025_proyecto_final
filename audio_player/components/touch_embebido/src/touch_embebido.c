// touch_embebido.c
#include "touch_embebido.h"
#include "esp_log.h"
#include "driver/touch_pad.h"
#include "esp_rom_delay_ms.h"

static const char *TAG = "touch_embebido";

#define NUM_CANALES 3

// photo, play/pause, network en el touch pad
static const touch_pad_t canales[NUM_CANALES] = {
    TOUCH_PAD_NUM2,
    TOUCH_PAD_NUM6,
    TOUCH_PAD_NUM11
};

static uint32_t umbrales[NUM_CANALES] = {0};

esp_err_t touch_embebido_iniciar(void) {
    ESP_LOGI(TAG, "Inicializando touch pad...");

    ESP_ERROR_CHECK(touch_pad_init());

    for (int i = 0; i < NUM_CANALES; i++) {
        ESP_ERROR_CHECK(touch_pad_config(canales[i]));
    }

    touch_pad_denoise_t denoise = {
        .grade = TOUCH_PAD_DENOISE_BIT4,
        .cap_level = TOUCH_PAD_DENOISE_CAP_L4,
    };
    touch_pad_denoise_set_config(&denoise);
    touch_pad_denoise_enable();

    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_filter_enable();
    touch_pad_fsm_start();

    esp_rom_delay_ms(300);

    // Calculamos umbrales
    for (int i = 0; i < NUM_CANALES; i++) {
        uint32_t base = 0;
        touch_pad_read_raw_data(canales[i], &base);
        umbrales[i] = base + 300;
        ESP_LOGI(TAG, "Canal %d (TOUCH%d): base=%lu, umbral=%lu",
                 i, canales[i], base, umbrales[i]);
    }

    return ESP_OK;
}

bool touch_embebido_fue_tocado(int canal) {
    if (canal < 0 || canal >= NUM_CANALES) return false; // Validación de canal

    uint32_t valor = 0;
    if (touch_pad_read_raw_data(canales[canal], &valor) == ESP_OK) {
        return valor > umbrales[canal];
    }

    return false;
}

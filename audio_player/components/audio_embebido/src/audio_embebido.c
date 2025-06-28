#include "audio_embebido.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "es8311.h"
#include "esp_log.h"
#include <string.h>

#define TAG "AUDIO_EMBEBIDO"

static i2s_chan_handle_t tx_handle = NULL;
static es8311_handle_t es_handle = NULL;

esp_err_t audio_embebido_iniciar(void) {
    // I2C init
    i2c_config_t i2c_cfg = {
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .mode = I2C_MODE_MASTER,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM, &i2c_cfg));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM, i2c_cfg.mode, 0, 0, 0));

    // Codec init
    es_handle = es8311_create(I2C_NUM, ES8311_ADDRRES_0);
    if (!es_handle) return ESP_FAIL;

    const es8311_clock_config_t clk_cfg = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = MCLK_FREQ_HZ,
        .sample_frequency = SAMPLE_RATE
    };

    ESP_ERROR_CHECK(es8311_init(es_handle, &clk_cfg, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
    ESP_ERROR_CHECK(es8311_sample_frequency_config(es_handle, MCLK_FREQ_HZ, SAMPLE_RATE));
    ESP_ERROR_CHECK(es8311_voice_volume_set(es_handle, VOICE_VOLUME, NULL));
    ESP_ERROR_CHECK(es8311_microphone_config(es_handle, false));
    
    // I2S TX init
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCK_IO,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        }
    };
    std_cfg.clk_cfg.mclk_multiple = MCLK_MULTIPLE;
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

    // PA enable
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = (1ULL << PA_CTRL_IO),
        .mode = GPIO_MODE_OUTPUT
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
    ESP_ERROR_CHECK(gpio_set_level(PA_CTRL_IO, 1));

    return ESP_OK;
}

void audio_embebido_reproducir(const uint8_t *ptr, size_t len) {
    size_t written = 0;

    while (1) {
        i2s_channel_write(tx_handle, ptr, len, &written, portMAX_DELAY);
        ESP_LOGI(TAG, "Reproducidos %d bytes", (int)written);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

i2s_chan_handle_t audio_embebido_get_tx_handle(void) {
    return tx_handle;
}

#include "audio_embebido.h"
#include "driver/i2s_std.h"
#include "driver/i2c_master.h" // modificar para que el código funciones con esta lib.
#include "driver/gpio.h"
#include "es8311.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_check.h"

#define TAG "AUDIO_EMBEBIDO"

#define I2S_SAMPLE_RATE   16000
#define I2S_BIT_WIDTH     I2S_DATA_BIT_WIDTH_16BIT
#define MCLK_MULTIPLE   (384) 
#define MCLK_FREQ_HZ    (I2S_SAMPLE_RATE * MCLK_MULTIPLE)

static i2s_chan_handle_t tx_chan = NULL;
// static i2s_chan_handle_t rx_chan = NULL;
static es8311_handle_t codec = NULL;

esp_err_t audio_embebido_iniciar(void) {
    esp_err_t err;

    // Configuración e instalación del driver I2C para el codec
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM, I2C_MODE_MASTER, 0, 0, 0));

    // Creación y configuración del codec (handler) ES8311
    codec = es8311_create(I2C_NUM_0, ES8311_ADDRRES_0);
    ESP_RETURN_ON_FALSE(codec, ESP_FAIL, TAG, "es8311 create failed");

    es8311_clock_config_t clk_cfg = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = MCLK_FREQ_HZ,
        .sample_frequency = I2S_SAMPLE_RATE
    };

    ESP_ERROR_CHECK(es8311_init(codec, &clk_cfg, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
    ESP_RETURN_ON_ERROR(es8311_sample_frequency_config(codec, I2S_SAMPLE_RATE * MCLK_MULTIPLE, I2S_SAMPLE_RATE), TAG, "set es8311 sample frequency failed");
    ESP_RETURN_ON_ERROR(es8311_voice_volume_set(codec, 80, NULL), TAG, "set es8311 volume failed");

    // Configuración del canal TX i2s
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    // Configuración del canal i2s
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCK_IO,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din = I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        }
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    // ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
    // ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
    
    // ESP_RETURN_ON_ERROR(es8311_microphone_config(codec, false), TAG, "set es8311 microphone failed");
    return ESP_OK;
}

esp_err_t audio_embebido_reproducir(const void *data, size_t size) {
    size_t bytes_escritos = 0;
    return i2s_channel_write(tx_chan, data, size, &bytes_escritos, pdMS_TO_TICKS(1000));
}
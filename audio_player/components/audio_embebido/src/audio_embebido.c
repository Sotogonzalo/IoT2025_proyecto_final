#include "audio_embebido.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "es8311.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <string.h>

#define TAG "AUDIO_EMBEBIDO"
#define MAX_CANCIONES 5
#define MAX_VOL 100
#define MIN_VOL 0
#define VOL_STEP 10

static i2s_chan_handle_t tx_handle = NULL;
static es8311_handle_t es_handle = NULL;

static char lista_paths[MAX_CANCIONES][64];  // Paths a archivos .pcm
static int total_canciones = 0;
static int cancion_actual = 0;

static audio_estado_t estado = AUDIO_STOPPED;
static int volumen_actual = 70;

static SemaphoreHandle_t mutex_estado;
static TaskHandle_t task_reproduccion = NULL;

static void tarea_reproduccion(void *param);

i2s_chan_handle_t audio_embebido_get_tx_handle(void) {
    return tx_handle;
}

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

    // pin PA para el stereo
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = (1ULL << PA_CTRL_IO),
        .mode = GPIO_MODE_OUTPUT
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
    ESP_ERROR_CHECK(gpio_set_level(PA_CTRL_IO, 1));

    // Init del mutex
    mutex_estado = xSemaphoreCreateMutex();

    return ESP_OK;
}

esp_err_t audio_embebido_cargar_lista(const char *const *paths, int cantidad) {
    if (cantidad > MAX_CANCIONES) return ESP_ERR_INVALID_ARG;

    for (int i = 0; i < cantidad; i++) {
        strncpy(lista_paths[i], paths[i], sizeof(lista_paths[i]));
    }

    total_canciones = cantidad;
    cancion_actual = 0;
    return ESP_OK;
}


void audio_embebido_play(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);
    if (estado == AUDIO_STOPPED || estado == AUDIO_PAUSED) {
        estado = AUDIO_PLAYING;
        if (task_reproduccion == NULL) {
            xTaskCreate(tarea_reproduccion, "tarea_audio", 4096, NULL, 5, &task_reproduccion);
        }
    }
    xSemaphoreGive(mutex_estado);
}

void audio_embebido_pause(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);
    if (estado == AUDIO_PLAYING) estado = AUDIO_PAUSED;
    xSemaphoreGive(mutex_estado);
}

void audio_embebido_stop(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);
    estado = AUDIO_STOPPED;
    xSemaphoreGive(mutex_estado);
}

void audio_embebido_next(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);
    cancion_actual = (cancion_actual + 1) % total_canciones;
    estado = AUDIO_PLAYING;
    xSemaphoreGive(mutex_estado);
}

void audio_embebido_prev(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);
    cancion_actual = (cancion_actual - 1 + total_canciones) % total_canciones;
    estado = AUDIO_PLAYING;
    xSemaphoreGive(mutex_estado);
}

void audio_embebido_volup(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);
    if (volumen_actual < MAX_VOL) volumen_actual += VOL_STEP;
    es8311_voice_volume_set(es_handle, volumen_actual, NULL);
    xSemaphoreGive(mutex_estado);
}

void audio_embebido_voldown(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);
    if (volumen_actual > MIN_VOL) volumen_actual -= VOL_STEP;
    es8311_voice_volume_set(es_handle, volumen_actual, NULL);
    xSemaphoreGive(mutex_estado);
}

static void tarea_reproduccion(void *param) {
    while (1) {
        xSemaphoreTake(mutex_estado, portMAX_DELAY);
        if (estado == AUDIO_STOPPED) {
            task_reproduccion = NULL;
            xSemaphoreGive(mutex_estado);
            vTaskDelete(NULL);
        } else if (estado == AUDIO_PAUSED) {
            xSemaphoreGive(mutex_estado);
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        char path_actual[64];
        strncpy(path_actual, lista_paths[cancion_actual], sizeof(path_actual));
        xSemaphoreGive(mutex_estado);

        FILE *f = fopen(path_actual, "rb");
        if (!f) {
            ESP_LOGE(TAG, "No se pudo abrir %s", path_actual);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        uint8_t buffer[512];
        size_t leido = 0;

        while ((leido = fread(buffer, 1, sizeof(buffer), f)) > 0) {
            xSemaphoreTake(mutex_estado, portMAX_DELAY);
            if (estado != AUDIO_PLAYING) {
                xSemaphoreGive(mutex_estado);
                break;
            }
            xSemaphoreGive(mutex_estado);

            size_t escrito = 0;
            i2s_channel_write(tx_handle, buffer, leido, &escrito, portMAX_DELAY);
        }

        fclose(f);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

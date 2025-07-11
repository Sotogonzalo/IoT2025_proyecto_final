#include "audio_embebido.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "es8311.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <string.h>
#include "config_embebido.h"
#include "event_logger.h"
#include "led_embebido.h"

#define TAG "AUDIO_EMBEBIDO"
#define MAX_CANCIONES 5
#define MAX_VOL 100
#define MIN_VOL 0
#define VOL_STEP 10

static i2s_chan_handle_t tx_handle = NULL;
static es8311_handle_t es_handle = NULL;

static size_t offset_actual = 0;
static char lista_paths[MAX_CANCIONES][64];  // Paths a archivos .pcm, todos almacenados en /spiffs
static int total_canciones = 0;
static int cancion_actual = 0;

static audio_estado_t estado = AUDIO_STOPPED;
static int volumen_actual = 50;

static SemaphoreHandle_t mutex_estado;
static TaskHandle_t task_reproduccion = NULL;
static TaskHandle_t task_parpadeo = NULL;

static void tarea_reproduccion(void *param);
static void tarea_chequeo_audio(void *param);

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

    // es8311 init
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

    // mutex init
    mutex_estado = xSemaphoreCreateMutex();

    // Creamos tarea del led al iniciar el audio
    if (task_parpadeo == NULL) {
        ESP_LOGI(TAG, "Creando tarea de parpadeo");
        xTaskCreate(tarea_chequeo_audio, "tarea_parpadeo_led", 4096, NULL, 2, &task_parpadeo);
    }

    return ESP_OK;
}

esp_err_t audio_embebido_cargar_lista(const char *const *paths, int cantidad) {
    if (cantidad > MAX_CANCIONES || cantidad < 0) return ESP_ERR_INVALID_ARG;

    // Limpia toda la lista
    for (int i = 0; i < MAX_CANCIONES; i++) {
        lista_paths[i][0] = '\0';
    }

    total_canciones = 0;

    for (int i = 0; i < cantidad; i++) {
        if (paths[i] == NULL || strlen(paths[i]) == 0) {
            ESP_LOGW(TAG, "Path inválido ignorado en índice %d", i);
            continue;
        }

        strncpy(lista_paths[total_canciones], paths[i], sizeof(lista_paths[0]) - 1);
        lista_paths[total_canciones][sizeof(lista_paths[0]) - 1] = '\0';
        total_canciones++;
    }

    // Muestra lista de reproducción
    ESP_LOGI(TAG, "Lista de reproducción cargada (%d canciones):", total_canciones);
    for (int i = 0; i < total_canciones; i++) {
        ESP_LOGI(TAG, "  [%d] %s", i, lista_paths[i]);
    }

    // Manejaa el caso de que no haya canciones válidas
    if (total_canciones == 0) {
        cancion_actual = 0;
        audio_embebido_stop();
        ESP_LOGI(TAG, "Lista vacía: reproducción detenida");
    } else {
        if (cancion_actual >= total_canciones || strlen(lista_paths[cancion_actual]) == 0) {
            cancion_actual = 0;
            ESP_LOGI(TAG, "Reinicio índice actual a 0");
        }
    }

    return ESP_OK;
}

// Un print diseñado para mostrar información de la canción actual y las siguientes, sólo para ver el estado de la lista y saber dónde se esta ubicado.
static void log_info_cancion(void) {
    ESP_LOGI(TAG, "Reproduciendo: %s", lista_paths[cancion_actual]);

    if (total_canciones > 1) {
        int anterior = (cancion_actual - 1 + total_canciones) % total_canciones;
        int siguiente = (cancion_actual + 1) % total_canciones;
        ESP_LOGI(TAG, "Anterior: %s", lista_paths[anterior]);
        ESP_LOGI(TAG, "Siguiente: %s", lista_paths[siguiente]);
    } else {
        ESP_LOGI(TAG, "No hay canciones anterior/siguiente");
    }
}

void audio_embebido_play(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);

    // Valida que haya canciones disponibles
    if (total_canciones == 0 || strlen(lista_paths[cancion_actual]) == 0) {
        ESP_LOGI(TAG, "No hay canciones en la lista de reproducción");
        xSemaphoreGive(mutex_estado);
        return;
    }

    // Si ya está reproduciendo, no hacemos nada
    if (estado == AUDIO_PLAYING) {
        ESP_LOGI(TAG, "Reproducción ya activa");
        xSemaphoreGive(mutex_estado);
        return;
    }

    estado = AUDIO_PLAYING;
    audio_embebido_guardar_estado();
    ESP_LOGI(TAG, "Reproducción iniciada o reanudada");

    event_logger_log_action("play", lista_paths[cancion_actual]);

    // Crear tarea de reproducción si no existe
    if (task_reproduccion == NULL) {
        ESP_LOGI(TAG, "Creando tarea de reproducción");
        xTaskCreate(tarea_reproduccion, "tarea_audio", 4096, NULL, 5, &task_reproduccion);
    }

    xSemaphoreGive(mutex_estado);
}

void audio_embebido_pause(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);
    if (estado == AUDIO_PLAYING) {
        estado = AUDIO_PAUSED;
        audio_embebido_guardar_estado();
        ESP_LOGI(TAG, "Reproducción pausada");
        event_logger_log_action("pause", lista_paths[cancion_actual]);
    } else {
        ESP_LOGI(TAG, "No se puede pausar: el audio no está en reproducción");
    }
    xSemaphoreGive(mutex_estado);
}

void audio_embebido_stop(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);
    if (estado == AUDIO_PLAYING || estado == AUDIO_PAUSED) {
        estado = AUDIO_STOPPED;
        offset_actual = 0;
        audio_embebido_guardar_estado();
        ESP_LOGI(TAG, "Reproducción detenida");

        event_logger_log_action("stop", lista_paths[cancion_actual]);
    } else {
        ESP_LOGI(TAG, "No se puede detener: el audio ya está detenido");
    }
    xSemaphoreGive(mutex_estado);
}

void audio_embebido_next(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);

    if (total_canciones <= 1) {
        ESP_LOGI(TAG, "No hay siguiente canción válida");
        xSemaphoreGive(mutex_estado);
        return;
    }

    // Buscar la siguiente canción válida
    // Comenzamos desde la canción actual y buscamos hacia adelante
    int nueva_pos = cancion_actual;
    for (int i = 1; i < MAX_CANCIONES; i++) {
        int idx = (cancion_actual + i) % MAX_CANCIONES;
        if (strlen(lista_paths[idx]) > 0) {
            nueva_pos = idx;
            break;
        }
    }

    // Si encontramos una canción diferente, actualizamos el índice
    // y reiniciamos el offset
    if (nueva_pos != cancion_actual) {
        cancion_actual = nueva_pos;
        offset_actual = 0;
        estado = AUDIO_STOPPED;
        ESP_LOGI(TAG, "Siguiente canción seleccionada: %s", lista_paths[cancion_actual]);
        event_logger_log_action("next", lista_paths[cancion_actual]);
    } else {
        ESP_LOGW(TAG, "No se encontró canción siguiente válida");
    }
    audio_embebido_guardar_estado();
    xSemaphoreGive(mutex_estado);
}

void audio_embebido_prev(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);

    if (total_canciones <= 1) {
        ESP_LOGI(TAG, "No hay canción anterior válida");
        xSemaphoreGive(mutex_estado);
        return;
    }

    // Buscar la canción anterior válida
    int nueva_pos = cancion_actual;
    for (int i = 1; i < MAX_CANCIONES; i++) {
        int idx = (cancion_actual - i + MAX_CANCIONES) % MAX_CANCIONES;
        if (strlen(lista_paths[idx]) > 0) {
            nueva_pos = idx;
            break;
        }
    }

    // Si cambiamos de canción, actualizamos el índice
    // y reiniciamos el offset
    if (nueva_pos != cancion_actual) {
        cancion_actual = nueva_pos;
        offset_actual = 0;
        estado = AUDIO_STOPPED;
        ESP_LOGI(TAG, "Canción anterior seleccionada: %s", lista_paths[cancion_actual]);
        event_logger_log_action("prev", lista_paths[cancion_actual]);
    } else {
        ESP_LOGW(TAG, "No se encontró canción anterior válida");
    }
    audio_embebido_guardar_estado();
    xSemaphoreGive(mutex_estado);
}

void audio_embebido_volup(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);
    if (volumen_actual < MAX_VOL) {
        volumen_actual += VOL_STEP;
        if (volumen_actual > MAX_VOL) volumen_actual = MAX_VOL;
        es8311_voice_volume_set(es_handle, volumen_actual, NULL);
        ESP_LOGI(TAG, "Volumen aumentado: %d", volumen_actual);
        event_logger_log_action("volUp", lista_paths[cancion_actual]);
    } else {
        ESP_LOGI(TAG, "Volumen máximo alcanzado: %d", volumen_actual);
    }
    xSemaphoreGive(mutex_estado);
}

void audio_embebido_voldown(void) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);
    if (volumen_actual > MIN_VOL) {
        volumen_actual -= VOL_STEP;
        if (volumen_actual < MIN_VOL) volumen_actual = MIN_VOL;
        es8311_voice_volume_set(es_handle, volumen_actual, NULL);
        ESP_LOGI(TAG, "Volumen reducido: %d", volumen_actual);
        event_logger_log_action("volDown", lista_paths[cancion_actual]);
    } else {
        ESP_LOGI(TAG, "Volumen mínimo alcanzado: %d", volumen_actual);
    }
    xSemaphoreGive(mutex_estado);
}

static void tarea_reproduccion(void *param) {
    xSemaphoreTake(mutex_estado, portMAX_DELAY);

    if (total_canciones == 0) {
        ESP_LOGW(TAG, "No hay canciones en la lista de reproducción.");
        task_reproduccion = NULL;
        offset_actual = 0;
        xSemaphoreGive(mutex_estado);
        vTaskDelete(NULL);
    }

    if (estado == AUDIO_STOPPED) {
        task_reproduccion = NULL;
        offset_actual = 0;
        xSemaphoreGive(mutex_estado);
        vTaskDelete(NULL);
    }

    char path_actual[64];
    strncpy(path_actual, lista_paths[cancion_actual], sizeof(path_actual));
    path_actual[sizeof(path_actual) - 1] = '\0';

    if (strlen(path_actual) == 0) {
        ESP_LOGE(TAG, "Canción actual vacía.");
        task_reproduccion = NULL;
        offset_actual = 0;
        xSemaphoreGive(mutex_estado);
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Iniciando reproducción: %s desde offset %d", path_actual, (int)offset_actual);
    log_info_cancion();

    xSemaphoreGive(mutex_estado);

    FILE *f = fopen(path_actual, "rb");
    if (!f) {
        ESP_LOGE(TAG, "No se pudo abrir %s", path_actual);
        task_reproduccion = NULL;
        offset_actual = 0;
        vTaskDelete(NULL);
    }

    // Nos posicionamos en offset_actual para retomar la reproducción
    if (fseek(f, offset_actual, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "Error al posicionar el archivo en offset %d", (int)offset_actual);
        fclose(f);
        task_reproduccion = NULL;
        offset_actual = 0;
        vTaskDelete(NULL);
    }

    uint8_t buffer[512];
    size_t leido = 0;

    while ((leido = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        xSemaphoreTake(mutex_estado, portMAX_DELAY);
        if (estado == AUDIO_PAUSED) {
            // Guardamos el offset actual para retomar después del pause
            offset_actual += ftell(f) - offset_actual;
            xSemaphoreGive(mutex_estado);
            break;
        }
        if (estado == AUDIO_STOPPED) {
            // Si se detuvo, salimos de la tarea
            fclose(f);
            task_reproduccion = NULL;
            offset_actual = 0;
            xSemaphoreGive(mutex_estado);
            vTaskDelete(NULL);
        }
        xSemaphoreGive(mutex_estado);

        size_t escrito = 0;
        i2s_channel_write(tx_handle, buffer, leido, &escrito, portMAX_DELAY);

        offset_actual += leido;  // Actualizamos el offset después de escribir
    }

    fclose(f);

    // Si terminó la canción o se detuvo por comando
    xSemaphoreTake(mutex_estado, portMAX_DELAY);
    if (estado == AUDIO_PLAYING) {
        // Canción terminada, reseteamos offset y estado, para que la sig. canción comience desde el inicio.
        offset_actual = 0;
        estado = AUDIO_STOPPED;
    }
    task_reproduccion = NULL;
    audio_embebido_guardar_estado();
    xSemaphoreGive(mutex_estado);

    ESP_LOGI(TAG, "Reproducción finalizada.");

    vTaskDelete(NULL);
}

void audio_embebido_guardar_estado(void) {
    configuracion_t cfg;
    if (!config_cargar(&cfg)) {
        memset(&cfg, 0, sizeof(cfg));
    }
    cfg.cancion_idx = cancion_actual;
    cfg.estado_audio = estado;
    cfg.offset_actual = offset_actual;

    for (int i = 0; i < MAX_CANCIONES; i++) {
        strncpy(cfg.canciones_actuales[i], lista_paths[i], sizeof(cfg.canciones_actuales[i]) - 1);
        cfg.canciones_actuales[i][sizeof(cfg.canciones_actuales[i]) - 1] = '\0';
    }

    if (!config_guardar(&cfg)) {
        ESP_LOGW(TAG, "No se pudo guardar estado audio");
    }
}

void audio_embebido_cargar_estado(void) {
    configuracion_t cfg;
    if (config_cargar(&cfg)) {
        cancion_actual = cfg.cancion_idx;
        estado = cfg.estado_audio;
        offset_actual = cfg.offset_actual;

        total_canciones = 0;
        for (int i = 0; i < MAX_CANCIONES; i++) {
            if (strlen(cfg.canciones_actuales[i]) > 0) {
                strncpy(lista_paths[total_canciones], cfg.canciones_actuales[i], sizeof(lista_paths[0]) - 1);
                lista_paths[total_canciones][sizeof(lista_paths[0]) - 1] = '\0';
                total_canciones++;
            } else {
                lista_paths[i][0] = '\0';
            }
        }

        ESP_LOGI(TAG, "Lista restaurada (%d canciones):", total_canciones);
        for (int i = 0; i < total_canciones; i++) {
            ESP_LOGI(TAG, "  [%d] %s", i, lista_paths[i]);
        }

    } else {
        ESP_LOGI(TAG, "No se pudo cargar estado y lista de canciones desde NVS");
    }
}

void tarea_chequeo_audio(void *param) {
    bool led_on = false;

    while (true) {
        xSemaphoreTake(mutex_estado, portMAX_DELAY);
        audio_estado_t estado_actual = estado;
        xSemaphoreGive(mutex_estado);

        switch (estado_actual) {
            case AUDIO_PLAYING:
                if (led_on) {
                    led_embebido_set_color(0, 255, 0); // Verde
                } else {
                    led_embebido_apagar();
                }
                vTaskDelay(pdMS_TO_TICKS(300));
                led_on = !led_on;
                break;

            case AUDIO_PAUSED:
                if (led_on) {
                    led_embebido_set_color(0, 0, 255); // Azul
                } else {
                    led_embebido_apagar();
                }
                vTaskDelay(pdMS_TO_TICKS(700));
                led_on = !led_on;
                break;

            case AUDIO_STOPPED:
                if (led_on) {
                    led_embebido_set_color(255, 0, 0); // Rojo
                } else {
                    led_embebido_apagar();
                }
                vTaskDelay(pdMS_TO_TICKS(500));
                led_on = !led_on;
                break;

            default:
                led_embebido_apagar();
                led_on = false;
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
        }
    }

    vTaskDelete(NULL);
}

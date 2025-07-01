#ifndef AUDIO_EMBEBIDO_H
#define AUDIO_EMBEBIDO_H

#include "esp_err.h"
#include "driver/i2s_std.h"

#define SAMPLE_RATE     (16000)
#define MCLK_MULTIPLE   (384)
#define MCLK_FREQ_HZ    (SAMPLE_RATE * MCLK_MULTIPLE)

#define I2C_NUM         (I2C_NUM_1)
#define I2C_SCL_IO      (GPIO_NUM_7)
#define I2C_SDA_IO      (GPIO_NUM_8)

#define I2S_NUM         (I2S_NUM_0)
#define I2S_MCK_IO      (GPIO_NUM_41)
#define I2S_BCK_IO      (GPIO_NUM_39)
#define I2S_WS_IO       (GPIO_NUM_21)
#define I2S_DO_IO       (GPIO_NUM_3)
#define I2S_DI_IO       (GPIO_NUM_1)

#define PA_CTRL_IO      (GPIO_NUM_10)
#define VOICE_VOLUME   (70)

typedef enum {
    AUDIO_STOPPED,
    AUDIO_PLAYING,
    AUDIO_PAUSED
} audio_estado_t;

i2s_chan_handle_t audio_embebido_get_tx_handle(void);
esp_err_t audio_embebido_iniciar(void);
esp_err_t audio_embebido_cargar_lista(const char *const *rutas_canciones, int cantidad);

void audio_embebido_play(void);
void audio_embebido_pause(void);
void audio_embebido_stop(void);
void audio_embebido_next(void);
void audio_embebido_prev(void);
void audio_embebido_volup(void);
void audio_embebido_voldown(void);

#endif // AUDIO_EMBEBIDO_H

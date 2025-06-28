#ifndef AUDIO_EMBEBIDO_H
#define AUDIO_EMBEBIDO_H

#include "esp_err.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/i2s_std.h"

#define SAMPLE_RATE     (16000)
#define MCLK_MULTIPLE   (384)
#define MCLK_FREQ_HZ    (SAMPLE_RATE * MCLK_MULTIPLE)
#define VOICE_VOLUME    (100)

#define I2C_NUM         (I2C_NUM_0)
#define I2C_SCL_IO      (GPIO_NUM_7)   // SCL I2C 
#define I2C_SDA_IO      (GPIO_NUM_8)   // SDA I2C

#define I2S_NUM         (I2S_NUM_0)
#define I2S_MCK_IO      (GPIO_NUM_41)   // MCLK
#define I2S_BCK_IO      (GPIO_NUM_39)   // BCLK
#define I2S_WS_IO       (GPIO_NUM_21)   // WS/LRCK
#define I2S_DO_IO       (GPIO_NUM_12)  // DOUT (DAC)
#define I2S_DI_IO       (GPIO_NUM_46)  // DIN  (ADC)

#define PA_CTRL_IO      (CONFIG_AUDIO_EMBEBIDO_PA_CTRL_IO)

esp_err_t audio_embebido_iniciar(void);
void audio_embebido_reproducir(const uint8_t *ptr, size_t len);
i2s_chan_handle_t audio_embebido_get_tx_handle(void);

#endif // AUDIO_EMBEBIDO_H

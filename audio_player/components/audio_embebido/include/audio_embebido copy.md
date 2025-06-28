#ifndef AUDIO_EMBEBIDO_H
#define AUDIO_EMBEBIDO_H

#include "esp_err.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define I2C_SCL_IO      (GPIO_NUM_7)   // SCL I2C 
#define I2C_SDA_IO      (GPIO_NUM_8)   // SDA I2C

#define I2S_MCK_IO      (GPIO_NUM_41)   // MCLK
#define I2S_BCK_IO      (GPIO_NUM_39)   // BCLK
#define I2S_WS_IO       (GPIO_NUM_21)   // WS/LRCK
#define I2S_DO_IO       (GPIO_NUM_12)  // DOUT (DAC)
#define I2S_DI_IO       (GPIO_NUM_46)  // DIN  (ADC)

#define I2C_NUM         (I2C_NUM_0)
#define I2S_NUM         (I2S_NUM_0)


esp_err_t audio_embebido_iniciar(void);
esp_err_t audio_embebido_reproducir(const void *datos, size_t longitud);


#endif // AUDIO_EMBEBIDO_H

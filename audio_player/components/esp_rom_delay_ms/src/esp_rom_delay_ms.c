#include <stdio.h>
#include "esp_rom_delay_ms.h"
#include "esp_rom_sys.h"

void esp_rom_delay_ms(int delay)
{
    int i;
    for (i = 0; i != 1000; i++)
    {
        esp_rom_delay_us(delay);
    }
}

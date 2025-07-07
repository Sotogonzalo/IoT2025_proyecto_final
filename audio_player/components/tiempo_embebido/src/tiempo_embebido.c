#include "tiempo_embebido.h"
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include "esp_sntp.h"
#include "esp_log.h"

#define TAG "TIEMPO_EMBEBIDO"

#define NTP_SERVIDOR "pool.ntp.org"
#define RETARDO_MAX_SYNC_MS 10000

static bool tiempo_sincronizado = false;

static void ntp_callback(struct timeval *tv)
{
    tiempo_sincronizado = true;
    ESP_LOGI(TAG, "Hora sincronizada correctamente con NTP");
}

void tiempo_embebido_iniciar(void)
{
    // Configuramos zona horaria a Uruguay
    setenv("TZ", "EST4EDT,M3.2.0/2,M11.1.0", 1);
    tzset();

    // Configuramos cliente SNTP
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, NTP_SERVIDOR);
    sntp_set_time_sync_notification_cb(ntp_callback);
    sntp_init();

    ESP_LOGI(TAG, "Esperando sincronización NTP...");
    int intentos = 0;
    while (!tiempo_sincronizado && intentos < RETARDO_MAX_SYNC_MS / 100)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        intentos++;
    }

    if (tiempo_sincronizado)
    {
        ESP_LOGI(TAG, "Hora sincronizada tras %d ms", intentos * 100);
    }
    else
    {
        ESP_LOGW(TAG, "No se pudo sincronizar la hora tras %d ms", RETARDO_MAX_SYNC_MS);
    }
}

bool tiempo_embebido_esta_sincronizado(void)
{
    return tiempo_sincronizado;
}

void tiempo_embebido_mostrar_hora_actual(void)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2020 - 1900))
    {
        ESP_LOGW(TAG, "La hora aún no fue sincronizada correctamente");
    }
    else
    {
        char buffer[64];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        ESP_LOGI(TAG, "Hora actual: %s", buffer);
    }
}

bool tiempo_embebido_obtener_timestamp(char *buffer, size_t max_len)
{
    if (!buffer || max_len == 0)
        return false;

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2020 - 1900))
    {
        return false; // no sincronizado aún
    }

    strftime(buffer, max_len, "%Y-%m-%d %H:%M:%S", &timeinfo);
    return true;
}

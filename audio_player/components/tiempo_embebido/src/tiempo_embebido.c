#include <time.h>
#include "tiempo_embebido.h"
#include "wifi_embebido.h"
#include <string.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"

// sincroniza la hora

void sincronizar_hora_ntp(void)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);
    int retry = 0;
    const int retry_count = 10;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count)
    {
        printf("Esperando NTP...\n");
    }
    esp_netif_sntp_deinit();
}
void obtener_timestamp_actual(char *destino, size_t max_len)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.t// guarda buffer circular en la flash
static void guardar_buffer_nvs()
{
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) != ESP_OK)
        return;
    nvs_set_blob(handle, "log_buffer", buffer_log, sizeof(buffer_log));
    nvs_set_u8(handle, "log_index", log_index);
    nvs_commit(handle);
    nvs_close(handle);
}
// carga el buffer ya guardado de la flash m_year < (2016 - 1900))
    {
        printf("⚠️ Hora no sincronizada aún. Sincronizando...\n");
        sincronizar_hora_ntp();
        time(&now);
        localtime_r(&now, &timeinfo);

        setenv("TZ", "EST4EDT,M3.2.0/2,M11.1.0", 1); // hora de Uruguay
        tzset();
    }

    strftime(destino, max_len, "%Y-%m-%d %H:%M:%S", &timeinfo);
}
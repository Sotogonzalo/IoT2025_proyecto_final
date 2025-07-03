#include "wifi_embebido.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include <stdbool.h>
#include <string.h>
#include "servidor_embebido.h"

static const char *TAG = "Módulo WiFi";

static EventGroupHandle_t wifi_event_group = NULL;
#define WIFI_CONNECTED_BIT BIT0

static esp_netif_t *wifi_sta_netif = NULL;
static esp_netif_t *wifi_ap_netif = NULL;

static bool inicializado = false;
static bool eventos_registrados = false;

esp_err_t iniciar_wifi_embebido(void) {
    if (inicializado) return ESP_OK;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    if (wifi_event_group == NULL) {
        wifi_event_group = xEventGroupCreate();
    }

    inicializado = true;
    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Intentando conectar a la red...");

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Conectado a la red WiFi");

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        if (wifi_event_group != NULL) {
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
            ESP_LOGI(TAG, "IP obtenida correctamente");

            iniciar_servidor_web();
        } else {
            ESP_LOGE(TAG, "wifi_event_group no inicializado.");
        }

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "Modo AP iniciado");

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        detener_servidor_web();

        esp_wifi_connect();
        if (wifi_event_group != NULL) {
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        } else {
            ESP_LOGE(TAG, "wifi_event_group no inicializado.");
        }

        ESP_LOGW(TAG, "Desconectado de WiFi. Reintentando conexión...");
    }
}


void iniciar_wifi_sta(const char *ssid, const char *password) {
    iniciar_wifi_embebido();

    if (wifi_sta_netif) {
        ESP_LOGI(TAG, "Destruyendo interfaz WiFi STA previa");
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy(wifi_sta_netif);
        wifi_sta_netif = NULL;
    }

    wifi_sta_netif = esp_netif_create_default_wifi_sta();
    if (!wifi_sta_netif) {
        ESP_LOGE(TAG, "No se pudo crear interfaz WiFi STA");
        return;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    if (!eventos_registrados) {
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
        eventos_registrados = true;
    }

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void iniciar_wifi_ap(const char *ssid, const char *password) {
    iniciar_wifi_embebido();

    if (wifi_ap_netif) {
        ESP_LOGI(TAG, "Destruyendo interfaz WiFi AP previa");
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy(wifi_ap_netif);
        wifi_ap_netif = NULL;
    }

    wifi_ap_netif = esp_netif_create_default_wifi_ap();
    if (!wifi_ap_netif) {
        ESP_LOGE(TAG, "No se pudo crear interfaz WiFi AP");
        return;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    if (!eventos_registrados) {
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
        eventos_registrados = true;
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = 0,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password));

    if (strlen(password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void reiniciar_wifi_sta(const char *ssid, const char *password) {
    ESP_LOGI(TAG, "Reiniciando WiFi con nuevas credenciales...");

    if (!inicializado) {
        iniciar_wifi_embebido();
    }

    if (wifi_sta_netif) {
        ESP_LOGI(TAG, "Deteniendo WiFi actual");
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy(wifi_sta_netif);
        wifi_sta_netif = NULL;
    }

    iniciar_wifi_sta(ssid, password);
}

bool wifi_esta_conectado(void) {
    if (wifi_event_group == NULL) {
        ESP_LOGE(TAG, "wifi_event_group no inicializado (func: wifi_esta_conectado)");
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        pdMS_TO_TICKS(10000)
    );

    return (bits & WIFI_CONNECTED_BIT) != 0;
}

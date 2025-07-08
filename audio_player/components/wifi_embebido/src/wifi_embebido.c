#include "wifi_embebido.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include "mqtt_embebido.h"
#include "config_embebido.h"
#include "servidor_embebido.h"

static const char *TAG = "WIFI_EMBEBIDO";

static EventGroupHandle_t wifi_event_group = NULL;
#define WIFI_CONNECTED_BIT BIT0

static esp_netif_t *wifi_sta_netif = NULL;
static esp_netif_t *wifi_ap_netif = NULL;

static bool inicializado = false;
static bool eventos_registrados = false;

esp_err_t iniciar_wifi_embebido(void) {
    if (inicializado) return ESP_OK;

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
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi STA iniciado");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WiFi STA conectado");
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "WiFi STA desconectado");
                if (wifi_event_group) {
                    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
                }
                break;
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "IP obtenida: " IPSTR, IP2STR(&event->ip_info.ip));

        if (wifi_event_group) {
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
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

// Tarea que reinicia el WiFi fuera del contexto de los handlers web
void tarea_reiniciar_wifi(void *param) {
    vTaskDelay(pdMS_TO_TICKS(500));  // Esperar a que el handler devuelva respuesta y libere socket

    configuracion_t cfg;
    config_cargar(&cfg);

    reiniciar_wifi_sta(cfg.ssid, cfg.password);

    vTaskDelete(NULL);
}

void reiniciar_wifi_sta(const char *ssid, const char *password) {
    ESP_LOGI(TAG, "Reiniciando WiFi con nuevas credenciales...");

    bool reiniciar_mqtt = mqtt_embebido_esta_conectado();

    if (reiniciar_mqtt) {
        ESP_LOGI(TAG, "Deteniendo MQTT antes del reinicio");
        mqtt_embebido_stop();
    }

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

    // Esperamos reconexión (máx. 10 segundos)
    int intentos = 10;
    while (intentos-- > 0 && !wifi_esta_conectado()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (!wifi_esta_conectado()) {
        ESP_LOGW(TAG, "No se pudo conectar a la nueva red WiFi.");
        return;
    }

    // Reiniciar servidor web solo si estaba activo (puedes agregar función para verificarlo)
    if (servidor_web_esta_activo()) {
        detener_servidor_web();
        iniciar_servidor_web();
    }

    if (reiniciar_mqtt) {
        configuracion_t cfg;
        config_cargar(&cfg);
        char uri_mqtt[128];
        snprintf(uri_mqtt, sizeof(uri_mqtt), "mqtt://%s:%d", cfg.mqtt_uri, cfg.mqtt_port);
        mqtt_embebido_start(uri_mqtt);
    }
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

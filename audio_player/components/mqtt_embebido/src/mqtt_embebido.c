#include "mqtt_embebido.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

#define BROKER_URI "mqtt://broker.hivemq.com"
#define MI_ID "ESP32S2_A"
#define TOPIC_RX "iot/demo/" MI_ID "/rx"

static const char *TAG = "MQTT_EMBEBIDO";
static esp_mqtt_client_handle_t client = NULL;

/* Callback del evento MQTT */
static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT conectado");
        esp_mqtt_client_subscribe(client, TOPIC_RX, 1);
        break;

    case MQTT_EVENT_DATA:
    {
        char topic[event->topic_len + 1];
        char data[event->data_len + 1];
        memcpy(topic, event->topic, event->topic_len);
        topic[event->topic_len] = '\0';
        memcpy(data, event->data, event->data_len);
        data[event->data_len] = '\0';

        ESP_LOGI(TAG, "RX [%s]: %s", topic, data);
        // Aquí podrías delegar el procesamiento a un callback externo
        break;
    }

    default:
        break;
    }
}

void mqtt_embebido_start(void)
{
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = BROKER_URI,
        .credentials.client_id = MI_ID};

    client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_embebido_publicar_json(const char *topico, const char *mensaje)
{
    if (client)
    {
        esp_mqtt_client_publish(client, topico, mensaje, 0, 1, 0);
        ESP_LOGI(TAG, "Publicado JSON en %s: %s", topico, mensaje);
    }
}

esp_mqtt_client_handle_t mqtt_embebido_get_client(void)
{
    return client;
}

#include "mqtt_embebido.h"
#include "audio_embebido.h"
#include "esp_log.h"
#include "cJSON.h"
// #include "logger.h" // FALTA

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t client = NULL;
static bool conectado = false;

#define MI_ID "ESP32S2_A"
#define TOPIC_RX "iot/demo/" MI_ID "/rx"

bool mqtt_embebido_esta_conectado(void) {
    return conectado;
}

static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        conectado = true;
        ESP_LOGI(TAG, "MQTT conectado");
        esp_mqtt_client_subscribe(client, TOPIC_RX, 1);
        // mqtt_embebido_enviar_logger_pendiente();  // Al conectarse, envia eventos pendientes de algun logger o queue de mensajes
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "ACK recibido para msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        conectado = false;
        ESP_LOGW(TAG, "MQTT desconectado");
        break;

    case MQTT_EVENT_DATA: {
        char topic[event->topic_len + 1];
        char data[event->data_len + 1];
        memcpy(topic, event->topic, event->topic_len); topic[event->topic_len] = '\0';
        memcpy(data, event->data, event->data_len); data[event->data_len] = '\0';

        ESP_LOGI(TAG, "RX [%s]: %s", topic, data);

        cJSON *root = cJSON_Parse(data);
        if (!root) {
            ESP_LOGW(TAG, "Payload no es JSON válido");
            break;
        }

        const cJSON *accion = cJSON_GetObjectItem(root, "accion");
        if (cJSON_IsString(accion)) {
            ESP_LOGI(TAG, "Acción MQTT: %s", accion->valuestring);

            if      (strcmp(accion->valuestring, "play") == 0)     audio_embebido_play();
            else if (strcmp(accion->valuestring, "pause") == 0)    audio_embebido_pause();
            else if (strcmp(accion->valuestring, "stop") == 0)     audio_embebido_stop();
            else if (strcmp(accion->valuestring, "next") == 0)     audio_embebido_next();
            else if (strcmp(accion->valuestring, "prev") == 0)     audio_embebido_prev();
            else if (strcmp(accion->valuestring, "volup") == 0)    audio_embebido_volup();
            else if (strcmp(accion->valuestring, "voldown") == 0)  audio_embebido_voldown();
            else ESP_LOGW(TAG, "Comando MQTT desconocido: %s", accion->valuestring);
        }

        cJSON_Delete(root);
        break;
    }

    default:
        break;
    }
}

void mqtt_embebido_start(const char *uri) {
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = uri,
        .credentials.client_id = MI_ID
    };

    client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_embebido_publicar_json(const char *topico, const char *mensaje) {
    if (client && conectado) {
        int msg_id = esp_mqtt_client_publish(client, topico, mensaje, 0, 1, 0);  // QoS 1
        ESP_LOGI(TAG, "Publicado JSON (msg_id=%d) en %s: %s", msg_id, topico, mensaje);
    }
}

#include "servidor_embebido.h"
#include "esp_log.h"
#include <stdio.h>
#include <dirent.h>
#include "esp_http_server.h"
#include "cJSON.h"
#include <string.h>
#include <sys/stat.h>
#include "esp_spiffs.h"
#include "audio_embebido.h"
#include "config_embebido.h"
#include "wifi_embebido.h"
#include "queue_embebido.h"
#include "event_logger.h"

extern void mqtt_embebido_start(const char *uri);

httpd_handle_t server = NULL;
#define HTTPD_507_INSUFFICIENT_STORAGE 507
static const char *TAG = "Módulo Servidor Web";
#define MAX_LINEA 272
#define MAX_CANCIONES 5  // Debe coincidir con el macro de audio embebido
#define MAX_PCMCANCION_BYTES (200 * 1024) // 200 KB máximo por canción PCM


void actualizar_lista_reproduccion(void) {
    static char canciones[MAX_CANCIONES][MAX_LINEA];
    const char *lista[MAX_CANCIONES];
    int count = 0;

    DIR *dir = opendir("/spiffs");
    if (!dir) {
        ESP_LOGE(TAG, "No se pudo abrir el directorio /spiffs");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < MAX_CANCIONES) {
        if (strstr(entry->d_name, ".pcm")) {
            snprintf(canciones[count], MAX_LINEA, "/spiffs/%s", entry->d_name);
            lista[count] = canciones[count];
            ESP_LOGI(TAG, "Canción [%d]: %s", count, lista[count]);
            count++;
        }
    }
    closedir(dir);

    audio_embebido_cargar_lista(lista, count);

    if (count > 0) {
        ESP_LOGI(TAG, "Lista de reproducción actualizada con %d canciones", count);
    } else {
        ESP_LOGW(TAG, "No se encontraron canciones .pcm en /spiffs");
    }

}

// Handler GET /
static esp_err_t raiz_get_handler(httpd_req_t *req) {
    FILE *f = fopen("/spiffs/index.html", "r");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "index.html no encontrado");
        return ESP_FAIL;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char *html = malloc(len + 1);
    if (!html) {
        fclose(f);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No hay memoria");
        return ESP_FAIL;
    }

    fread(html, 1, len, f);
    html[len] = '\0';
    fclose(f);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, len);
    free(html);
    return ESP_OK;
}

// Handler POST /config_wifi
static esp_err_t config_wifi_post_handler(httpd_req_t *req) {
    char buf[256];
    int total_len = req->content_len;
    if (total_len >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload demasiado grande");
        return ESP_FAIL;
    }

    int recv_len = httpd_req_recv(req, buf, total_len);
    if (recv_len <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error al recibir");
        return ESP_FAIL;
    }
    buf[recv_len] = '\0';
    ESP_LOGI(TAG, "JSON WiFi recibido: %s", buf);

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "JSON inválido");
        return ESP_FAIL;
    }

    const cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    const cJSON *password = cJSON_GetObjectItem(root, "password");

    if (!cJSON_IsString(ssid) || !cJSON_IsString(password)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Campos faltantes en WiFi");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    configuracion_t cfg;
    config_cargar(&cfg); // mantener mqtt_uri y puerto

    bool cambiaron_credenciales =
        strncmp(cfg.ssid, ssid->valuestring, CONFIG_MAX_STR) != 0 ||
        strncmp(cfg.password, password->valuestring, CONFIG_MAX_STR) != 0;

    strncpy(cfg.ssid, ssid->valuestring, CONFIG_MAX_STR);
    strncpy(cfg.password, password->valuestring, CONFIG_MAX_STR);

    if (!config_guardar(&cfg)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No se pudo guardar WiFi");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "WiFi actualizado: SSID=%s", cfg.ssid);

    if (cambiaron_credenciales) {
        ESP_LOGI(TAG, "Reiniciando WiFi con nuevas credenciales...");
        reiniciar_wifi_sta(cfg.ssid, cfg.password);

        // Esperamos a que conecte (máx. 10 segundos)
        int timeout = 10;
        while (timeout > 0 && !wifi_esta_conectado()) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            timeout--;
        }

        if (!wifi_esta_conectado()) {
            ESP_LOGW(TAG, "No se pudo conectar al nuevo WiFi. Por favor, envíe nuevas credenciales.");
            
            httpd_resp_set_status(req, "408 Request Timeout");
            httpd_resp_send(req, "No se pudo conectar al nuevo WiFi", HTTPD_RESP_USE_STRLEN);
            cJSON_Delete(root);
            
            return ESP_FAIL;
        }


    } else {
        ESP_LOGI(TAG, "Las credenciales no cambiaron, sin reiniciar WiFi.");
    }

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "WiFi guardado correctamente");
    return ESP_OK;
}


// Handler POST /config_mqtt
static esp_err_t config_mqtt_post_handler(httpd_req_t *req) {
    char buf[256];
    int total_len = req->content_len;
    if (total_len >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload demasiado grande");
        return ESP_FAIL;
    }

    int recv_len = httpd_req_recv(req, buf, total_len);
    if (recv_len <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error al recibir");
        return ESP_FAIL;
    }
    buf[recv_len] = '\0';
    ESP_LOGI(TAG, "JSON MQTT recibido: %s", buf);

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "JSON inválido");
        return ESP_FAIL;
    }

    const cJSON *mqtt_uri = cJSON_GetObjectItem(root, "mqtt_uri");
    const cJSON *mqtt_port = cJSON_GetObjectItem(root, "mqtt_port");

    if (!cJSON_IsString(mqtt_uri) || !cJSON_IsNumber(mqtt_port)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Campos faltantes en MQTT");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    configuracion_t cfg;
    config_cargar(&cfg); // mantener ssid y password actuales

    strncpy(cfg.mqtt_uri, mqtt_uri->valuestring, CONFIG_MAX_STR);
    cfg.mqtt_port = mqtt_port->valueint;

    if (!config_guardar(&cfg)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No se pudo guardar MQTT");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MQTT actualizado: %s:%d", cfg.mqtt_uri, cfg.mqtt_port);

    char uri_mqtt[128];
    snprintf(uri_mqtt, sizeof(uri_mqtt), "mqtt://%s:%d", cfg.mqtt_uri, cfg.mqtt_port);
    mqtt_embebido_start(uri_mqtt);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "MQTT guardado correctamente");
    return ESP_OK;
}

// Handler POST /comando
static esp_err_t comando_post_handler(httpd_req_t *req) {
    char buf[128];
    int total_len = req->content_len;

    if (total_len >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload demasiado grande");
        return ESP_FAIL;
    }

    int recv_len = httpd_req_recv(req, buf, total_len);
    if (recv_len <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error al recibir");
        return ESP_FAIL;
    }

    buf[recv_len] = '\0';
    ESP_LOGI(TAG, "Comando recibido: %s", buf);

    cJSON *root = cJSON_Parse(buf);
    const cJSON *accion = cJSON_GetObjectItem(root, "accion");

    if (cJSON_IsString(accion)) {
        ESP_LOGI(TAG, "Acción: %s", accion->valuestring);

        music_command_t cmd = parse_command(accion->valuestring);
        if (cmd != CMD_INVALID) {
            push_command(cmd);
        } else {
            ESP_LOGW(TAG, "Comando desconocido: %s", accion->valuestring);
        }
    }

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Comando recibido");
    return ESP_OK;
}

// Handler POST /ver_log
static esp_err_t ver_log_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Solicitud /ver_log recibida, imprimiendo log en consola...");
    event_logger_print();

    httpd_resp_sendstr(req, "Log impreso en consola");
    return ESP_OK;
}

// Handler POST /upload
static esp_err_t upload_pcm_handler(httpd_req_t *req) {
    static int contador = 0;
    ESP_LOGI(TAG, "Inicio handler /upload_pcm");

    if (req->content_len > MAX_PCMCANCION_BYTES) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Archivo demasiado grande");
        return ESP_FAIL;
    }

    // Verificar espacio en SPIFFS
    size_t total_spiffs = 0, used_spiffs = 0;
    esp_err_t ret = esp_spiffs_info(NULL, &total_spiffs, &used_spiffs);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo obtener info SPIFFS (%s)", esp_err_to_name(ret));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "SPIFFS no disponible");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "SPIFFS Total: %d bytes, Usado: %d bytes, Libre: %d bytes", total_spiffs, used_spiffs, total_spiffs - used_spiffs);

    if (total_spiffs - used_spiffs < 320000) {
        ESP_LOGE(TAG, "No hay suficiente espacio en SPIFFS para el archivo");
        httpd_resp_send_err(req, HTTPD_507_INSUFFICIENT_STORAGE, "Espacio insuficiente en SPIFFS");
        return ESP_FAIL;
    }

    // Construir ruta de archivo
    char filename[64];
    snprintf(filename, sizeof(filename), "/spiffs/song%d.pcm", contador);
    ESP_LOGI(TAG, "Creando archivo: %s", filename);

    FILE *f = fopen(filename, "wb");
    if (!f) {
        ESP_LOGE(TAG, "fopen falló para %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No se pudo crear archivo");
        return ESP_FAIL;
    }

    char buf[512];
    int total = 0;
    int leido = 0;

    // Leer datos del POST
    while ((leido = httpd_req_recv(req, buf, sizeof(buf))) > 0) {
        size_t escritos = fwrite(buf, 1, leido, f);
        if (escritos < (size_t)leido) {
            ESP_LOGE(TAG, "Error escribiendo archivo");
            fclose(f);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error escribiendo archivo");
            return ESP_FAIL;
        }
        total += escritos;
    }

    if (leido < 0) {
        ESP_LOGE(TAG, "Error recibiendo datos del cliente");
        fclose(f);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error recibiendo datos");
        return ESP_FAIL;
    }

    fclose(f);
    ESP_LOGI(TAG, "Archivo recibido: %s (%d bytes)", filename, total);

    contador++;

    actualizar_lista_reproduccion();

    httpd_resp_sendstr(req, "Canción subida correctamente");

    DIR *dir = opendir("/spiffs");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            ESP_LOGI(TAG, "Archivo en SPIFFS: %s", ent->d_name);
        }
        closedir(dir);
    } else {
        ESP_LOGE(TAG, "No se pudo abrir /spiffs");
    }

    return ESP_OK;
}

// Handler GET /canciones
static esp_err_t canciones_get_handler(httpd_req_t *req) {
    DIR *dir = opendir("/spiffs");
    if (!dir) {
        ESP_LOGE(TAG, "No se pudo abrir /spiffs para listar canciones");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No se pudo abrir SPIFFS");
        return ESP_FAIL;
    }

    cJSON *json_array = cJSON_CreateArray();
    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {
        if (strstr(ent->d_name, ".pcm")) {
            char path[264];
            snprintf(path, sizeof(path), "/spiffs/%s", ent->d_name);
            cJSON_AddItemToArray(json_array, cJSON_CreateString(path));
        }
    }
    closedir(dir);

    char *respuesta = cJSON_PrintUnformatted(json_array);
    cJSON_Delete(json_array);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, respuesta);
    free(respuesta);

    return ESP_OK;
}

// Handler POST /delete_song
static esp_err_t delete_song_handler(httpd_req_t *req) {
    char path[64];
    int len = httpd_req_recv(req, path, sizeof(path) - 1);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Nombre de archivo no recibido");
        return ESP_FAIL;
    }

    path[len] = '\0';
    ESP_LOGI(TAG, "Solicitando eliminación: %s", path);

    // Verificar que el archivo esté dentro de /spiffs y termine en .pcm
    if (strncmp(path, "/spiffs/", 8) != 0 || !strstr(path, ".pcm")) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Nombre de archivo inválido");
        return ESP_FAIL;
    }

    // Intentar eliminar el archivo
    if (remove(path) != 0) {
        ESP_LOGW(TAG, "No se pudo eliminar archivo: %s", path);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No se pudo eliminar el archivo");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Archivo eliminado: %s", path);

    actualizar_lista_reproduccion();

    httpd_resp_sendstr(req, "Canción eliminada correctamente");
    return ESP_OK;
}

// Inicialización del servidor HTTP
httpd_handle_t iniciar_servidor_web(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (server != NULL) {
        ESP_LOGW(TAG, "Servidor web ya iniciado. Deteniéndolo antes de reiniciar.");
        detener_servidor_web();
    }

    if (httpd_start(&server, &config) == ESP_OK) {
        // GET /
        httpd_register_uri_handler(server, &(httpd_uri_t){
            .uri = "/", .method = HTTP_GET, .handler = raiz_get_handler });

        // POST /config_wifi
        httpd_register_uri_handler(server, &(httpd_uri_t){
            .uri = "/config_wifi", .method = HTTP_POST, .handler = config_wifi_post_handler });

        // POST /config_mqtt
        httpd_register_uri_handler(server, &(httpd_uri_t){
            .uri = "/config_mqtt", .method = HTTP_POST, .handler = config_mqtt_post_handler });

        // POST /comando
        httpd_register_uri_handler(server, &(httpd_uri_t){
            .uri = "/comando", .method = HTTP_POST, .handler = comando_post_handler });

        // POST /ver_log
        httpd_register_uri_handler(server, &(httpd_uri_t){
            .uri = "/ver_log", .method = HTTP_POST, .handler = ver_log_post_handler, .user_ctx = NULL});

        // POST /upload_pcm
        httpd_register_uri_handler(server, &(httpd_uri_t){
            .uri = "/upload_pcm", .method = HTTP_POST, .handler = upload_pcm_handler, .user_ctx = NULL });
        
        // POST /delete_song
        httpd_register_uri_handler(server, &(httpd_uri_t){
            .uri = "/delete_song", .method = HTTP_POST, .handler = delete_song_handler });

        // GET /canciones
        httpd_register_uri_handler(server, &(httpd_uri_t){
            .uri = "/canciones", .method = HTTP_GET, .handler = canciones_get_handler });

        ESP_LOGI(TAG, "Servidor web iniciado con éxito");
    } else {
        ESP_LOGE(TAG, "Error al iniciar el servidor web");
    }

    return server;
}

void detener_servidor_web(void) {
    if (server != NULL) {
        ESP_LOGI(TAG, "Deteniendo servidor web...");
        httpd_stop(server);
        server = NULL;
    } else {
        ESP_LOGW(TAG, "Servidor web ya detenido o no inicializado");
    }
}
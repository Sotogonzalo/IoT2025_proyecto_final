#include "servidor_embebido.h"
#include "esp_log.h"
#include "esp_http_server.h"

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

static const char *TAG = "ServidorWeb";

esp_err_t raiz_get_handler(httpd_req_t *req) {
    const uint32_t html_len = index_html_end - index_html_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, html_len);
    return ESP_OK;
}

httpd_handle_t iniciar_servidor_web(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_raiz = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = raiz_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_raiz);
        ESP_LOGI(TAG, "Servidor web iniciado con éxito");
    } else {
        ESP_LOGE(TAG, "Error al iniciar el servidor web");
    }

    return server;
}

/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "esp_tls_crypto.h"
#include <esp_http_server.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"

static const char *TAG = "http_server";

// HTTP request handler for the root URL "/"

esp_err_t root_handler(httpd_req_t *req) {
    // TODO: Add fourth text field for MQTT(S) broker
    const char *html_response = "<html><body>"
                                "<h1>ESP32 Webserver for Layblar Setup</h1>"
                                "<form action='/submit' method='post'>"
                                "   <label for='textfield1'>Wifi SSID:</label>"
                                "   <input type='text' id='textfield1' name='textfield1'><br>"
                                "   <label for='textfield2'>Wifi Password:</label>"
                                "   <input type='password' id='textfield2' name='textfield2'><br>"
                                "   <label for='textfield3'>Smart Meter Key:</label>"
                                "   <input type='text' id='textfield3' name='textfield3'><br>"
                                "   <input type='submit' value='Submit'>"
                                "</form>"
                                "</body></html>";

    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// HTTP request handler for form submission
esp_err_t submit_handler(httpd_req_t *req) {
    char content[300];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        // Read the data for the request
        if ((ret = httpd_req_recv(req, content, MIN(remaining, sizeof(content)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                // Retry receiving if timeout occurred
                continue;
            }
            return ESP_FAIL;
        }

        // Process the received data (you can save it to a variable, etc.)
        remaining -= ret;
    }

    // TODO: Unparse the received content (smart meter key & wifi ssid/pw) and implement them
    printf("Received: %s\n", content);

    // Respond to the client
    httpd_resp_send(req, "Data received successfully", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// HTTP server config
httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_handler,
    .user_ctx  = NULL
};

httpd_uri_t submit_uri = {
    .uri       = "/submit",
    .method    = HTTP_POST,
    .handler   = submit_handler,
    .user_ctx  = NULL
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");

        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &submit_uri);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

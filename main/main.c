#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_wifi.h"
#include <esp_log.h>
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "mqtt_client.h"

#define UART_PORT UART_NUM_1
#define RX 20
#define TX 21
#define BUF_SIZE 256
#define OUT_PIN GPIO_NUM_10

#define WIFI_SSID "Soi13"
#define WIFI_PASS ""

// MQTT Server/Broker credentials
#define MQTT_BROKER_URI "mqtt://192.168.1.64"
#define MQTT_USER "mqtt_user"
#define MQTT_PASSWORD ""
#define PRESENCE "homeassistant/sensor/presence"
#define PRESENCE_TYPE_STATIC "homeassistant/sensor/presence_static_target"
#define PRESENCE_TYPE_MOVING "homeassistant/sensor/presence_moving_target"

static const char *TAG = "LD2410";

//Configuration of GPIO for using OUT Pin on LD2410C
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << OUT_PIN),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
};

// Wifi event handler for displaying parameters of connection
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "Wi-Fi disconnected, retrying to reconnect. The reason is %d ", event->reason);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Wi-Fi connected");
        ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Subnet Mask: " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
    }
}

// Initializing Wifi connection
static void wifi_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(40)); //This method specifically for ESP32-C3, otherwise it will not connect to WiFi.
}

//Initializing MQTT
static esp_mqtt_client_handle_t client = NULL;

static void mqtt_app(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.username = MQTT_USER,
        .credentials.authentication.password = MQTT_PASSWORD,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

//UART initializing
void uart_init_ld2410()
{
    uart_config_t uart_config = {
        .baud_rate = 256000,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE * 8, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

//This method pasre raw response from LD2410C
void parse_ld2410(uint8_t *data, int len)
{
    char moving_dist[6], static_dist[6];

    for (int i = 0; i < len - 8; i++) {
        if (data[i] == 0xF4 && data[i+1] == 0xF3 &&
            data[i+2] == 0xF2 && data[i+3] == 0xF1) {

            // Example: presence flag at known offset (varies by mode)
            uint8_t target_status = data[i + 8];

            uint16_t moving_distance = data[9] | (data[10] << 8);
            uint16_t static_distance = data[12] | (data[13] << 8);

            if (target_status == 0x03) {
                snprintf(moving_dist, sizeof(moving_dist), "%u", moving_distance);
                ESP_LOGI(TAG, "Moving target detected. Distance cm: %d", moving_distance);
                esp_mqtt_client_publish(client, PRESENCE_TYPE_MOVING, moving_dist, 0, 1, 0);
            } else if (target_status == 0x02) {
                snprintf(static_dist, sizeof(static_dist), "%u", static_distance);
                ESP_LOGI(TAG, "Static target detected. Distance cm: %d", static_distance);
                esp_mqtt_client_publish(client, PRESENCE_TYPE_STATIC, static_dist, 0, 1, 0);
            } else {
                ESP_LOGW(TAG, "No target");
                esp_mqtt_client_publish(client, PRESENCE_TYPE_MOVING, "0", 0, 1, 0);
                esp_mqtt_client_publish(client, PRESENCE_TYPE_STATIC, "0", 0, 1, 0);
            }
        }
    }
}

void ld2410_task(void *arg)
{
    uint8_t data[BUF_SIZE];

    while (1) {
        int len = uart_read_bytes(UART_PORT, data, BUF_SIZE, pdMS_TO_TICKS(100));
        int state = gpio_get_level(OUT_PIN);

        if (len > 0) {
            parse_ld2410(data, len);
        }

        if (state) {
            printf("Presence detected\n");
            esp_mqtt_client_publish(client, PRESENCE, "Presence detected", 0, 1, 0);
        } else {
            printf("No presence\n");
            esp_mqtt_client_publish(client, PRESENCE, "No presence", 0, 1, 0);
        }
    }
}

void app_main(void)
{
    gpio_config(&io_conf);
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
    vTaskDelay(pdMS_TO_TICKS(5000));
    mqtt_app();
    vTaskDelay(pdMS_TO_TICKS(1000));
    uart_init_ld2410();
    xTaskCreate(ld2410_task, "ld2410_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "LD2410 task started");
}

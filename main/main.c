#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_wifi.h"
#include <esp_log.h>
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "mqtt_client.h"

#define UART_PORT UART_NUM_2
#define RX 16
#define TX 17
#define BUF_SIZE 256
#define OUT_PIN GPIO_NUM_4

#define WIFI_SSID "Soi13"
#define WIFI_PASS ""

// MQTT Server/Broker credentials
#define MQTT_BROKER_URI "mqtt://192.168.1.64"
#define MQTT_USER "mqtt_user"
#define MQTT_PASSWORD ""
#define PRESENCE "homeassistant/sensor/presence"
#define PRESENCE_TYPE_DISTANCE "homeassistant/sensor/presence_type_distance"

static const char *TAG = "LD2410";



/////////##Configuration section for LD2410C sensor##//////////////

//Sending request
void ld2410_send(uint8_t *cmd, size_t len) {
    uart_write_bytes(UART_NUM_2, (const char*)cmd, len);
}

//Reading response
int ld2410_read(uint8_t *buf, size_t max_len) {
    return uart_read_bytes(UART_NUM_2, buf, max_len, pdMS_TO_TICKS(200));
}

//Enter in configuration mode
uint8_t enter_config[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x04, 0x00,
    0xFF, 0x00,
    0x01, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Exit from configuration mode
uint8_t exit_config[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00,
    0xFE, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Read firmware version. Command: 0xA0
uint8_t read_version[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00,
    0xA0, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Read all parameters. Command: 0x61
//This returns: max distance, gate sensitivities, thresholds
uint8_t read_all[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00,
    0x61, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Enabling engineering mode. Command: 0x62
uint8_t enable_engineering_mode[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00,
    0x62, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Disabling engineering mode. Command: 0x63
uint8_t disable_engineering_mode[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00,
    0x63, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Set max detection distance. Command: 0x60
//We set here maximum distance gate 8 (stationary and motion). No one duration is 5 sec.
uint8_t max_distance_gate[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x14, 0x00,
    0x60, 0x00,
    0x00, 0x00,
    0x08, 0x00, 0x00, 0x00,
    0x01, 0x00,
    0x08, 0x00, 0x00, 0x00,
    0x02, 0x00,
    0x05, 0x00, 0x00, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Set gate sensetivity. Command: 0x64
void set_gate(uint8_t gate, uint8_t moving, uint8_t stat) {
    uint8_t cmd[] = {
        0xFD, 0xFC, 0xFB, 0xFA,
        0x14, 0x00,
        0x64, 0x00,
        0x00, 0x00,
        gate, 0x00, 0x00, 0x00,
        0x01, 0x00,
        moving, 0x00, 0x00, 0x00,
        0x02, 0x00,
        stat, 0x00, 0x00, 0x00,
        0x04, 0x03, 0x02, 0x01
    };

    ld2410_send(cmd, sizeof(cmd));
}

//Set all gates at once
void configure_all_gates() {
    // Near field → high sensitivity
    set_gate(0, 50, 100);
    set_gate(1, 50, 100);
    set_gate(2, 40, 40);

    // Mid range
    set_gate(3, 30, 40);
    set_gate(4, 20, 30);

    // Far range → suppress noise
    set_gate(5, 15, 30);
    set_gate(6, 15, 20);
    set_gate(7, 15, 20);
    set_gate(8, 15, 20);
}

//Start config main method
void ld2410_start_config() {

    uint8_t rx[256];

    uart_flush(UART_PORT);

    //Enter config mode
    ld2410_send(enter_config, sizeof(enter_config));
    vTaskDelay(pdMS_TO_TICKS(200));

    //ld2410_send(read_all, sizeof(read_all));
    configure_all_gates();
    vTaskDelay(pdMS_TO_TICKS(200));

    //Enable engineering mode
    //ld2410_send(enable_engineering_mode, sizeof(enable_engineering_mode));
    //vTaskDelay(pdMS_TO_TICKS(200));

    //Disable engineering mode
    //ld2410_send(disable_engineering_mode, sizeof(disable_engineering_mode));
    //vTaskDelay(pdMS_TO_TICKS(200));

    //Check response after executing configuration comand
    int len = ld2410_read(rx, sizeof(rx));

    if (len > 0) {
        ESP_LOGI(TAG, "Received %d bytes", len);

        for (int i = 0; i < len; i++) {
            printf("%02X ", rx[i]);
        }
        printf("\n");
    }

    //Exit config mode
    ld2410_send(exit_config, sizeof(exit_config));
    vTaskDelay(pdMS_TO_TICKS(200));
}

////////##End of configuration section for LD2410C sensor##////////



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
        ESP_LOGW(TAG, "Wi-Fi disconnected, retrying...");
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
    esp_wifi_start();
}

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


void parse_ld2410(uint8_t *data, int len)
{
    for (int i = 0; i < len - 8; i++) {
        if (data[i] == 0xF4 && data[i+1] == 0xF3 &&
            data[i+2] == 0xF2 && data[i+3] == 0xF1) {

            // Example: presence flag at known offset (varies by mode)
            uint8_t target_status = data[i + 8];

            uint16_t moving_distance = data[9] | (data[10] << 8);
            uint16_t static_distance = data[12] | (data[13] << 8);

            if (target_status == 0x03) {
                ESP_LOGI(TAG, "Moving target detected. Distance cm: %d", moving_distance);
            } else if (target_status == 0x02) {
                ESP_LOGI(TAG, "Static target detected. Distance cm: %d", static_distance);
            } else {
                ESP_LOGW(TAG, "No target");
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
        } else {
            printf("No presence\n");
        }
    }
}

void app_main(void)
{
    gpio_config(&io_conf);
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
    vTaskDelay(pdMS_TO_TICKS(5000));
    uart_init_ld2410();
    //ld2410_start_config();
    xTaskCreate(ld2410_task, "ld2410_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "LD2410 task started");
}

#include <stdio.h>
#include <esp_log.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define UART_PORT UART_NUM_1
#define RX 16
#define TX 17
#define BUF_SIZE 256

#define TAG "LD2410"

void uart_init_ld2410()
{
    uart_config_t uart_config = {
        .baud_rate = 256000,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_PORT, BUF_SIZE * 8, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
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
                ESP_LOGI("LD2410", "Moving target detected. Distance cm: %d", moving_distance);
            } else if (target_status == 0x02) {
                ESP_LOGI("LD2410", "Static target detected. Distance cm: %d", static_distance);
            } else {
                ESP_LOGW("LD2410", "No target");
            }
        }
    }
}

void ld2410_task(void *arg)
{
    uint8_t data[BUF_SIZE];

    while (1) {
        int len = uart_read_bytes(UART_PORT, data, BUF_SIZE, pdMS_TO_TICKS(100));

        if (len > 0) {
            parse_ld2410(data, len);
        }
    }
}

void app_main(void)
{
    uart_init_ld2410();
    xTaskCreate(ld2410_task, "ld2410_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "LD2410 task started");
    //vTaskDelay(pdMS_TO_TICKS(1000));
}

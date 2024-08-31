#include "serial.h"
#include <cstring>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart_vfs.h"
#include "driver/uart.h"

static const char* TAG = "uart_select_example";
static const int RX_BUF_SIZE = 1024;

#define TXD_PIN 18
#define RXD_PIN 5

Serial::Serial() {
    RX_BUF_SIZE = 1024;
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

Serial::~Serial() {}


int Serial::sendData(const char* data, const int lenght)
{
    return uart_write_bytes(UART_NUM_1, data, lenght);
}

int Serial::RecvData(char* data, const int lenght)
{
    return uart_read_bytes(UART_NUM_1, data, lenght, 1000 / portTICK_PERIOD_MS);
}

#include "serial.h"
#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/unistd.h>

static const char *TAG = "uart_select";

#define TXD_PIN 18
#define RXD_PIN 5

Serial::Serial(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num)
    : uart_num_(uart_num),
      tx_io_num_(tx_io_num),
      rx_io_num_(rx_io_num),
      rts_io_num_(rts_io_num),
      cts_io_num_(cts_io_num)
{
    rx_buf_size_ = 1024;
    uart_set_pin(uart_num_, tx_io_num, rx_io_num, rts_io_num, cts_io_num);
    if (uart_driver_install(uart_num_, 2 * 1024, 0, 0, NULL, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Driver installation failed");
        vTaskDelete(NULL);
    }

    uart_config_t uart_config = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(uart_num_, &uart_config);

    if ((uart_fd_ = open("/dev/uart/0", O_RDWR)) == -1) {
        ESP_LOGE(TAG, "Cannot open UART");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    // We have a driver now installed so set up the read/write functions to use driver also.
    uart_vfs_dev_use_driver(0);
}

Serial::~Serial()
{
    if (uart_fd_ > 0) {
        close(uart_fd_);
    }
}

int Serial::SendData(const char *data, const int lenght)
{
    return uart_write_bytes(uart_num_, data, lenght);
}

int Serial::RecvData(char *data, const int lenght)
{
    return uart_read_bytes(uart_num_, data, lenght, 1000 / portTICK_PERIOD_MS);
}

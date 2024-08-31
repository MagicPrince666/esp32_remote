#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <iostream>
#include <memory>
#include "driver/uart_vfs.h"
#include "driver/uart.h"

class Serial
{
private:
    int rx_buf_size_;
    char *rx_buf_;
    uart_port_t uart_num_;
    int uart_fd_;
    int tx_io_num_;
    int rx_io_num_;
    int rts_io_num_;
    int cts_io_num_;

    void AsyncRecvData();

public:
    Serial(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num = UART_PIN_NO_CHANGE, int cts_io_num = UART_PIN_NO_CHANGE);
    ~Serial();

    int SendData(const char* data, const int lenght);

    int RecvData(char* data, const int lenght);

    void AsyncRecv();
};

#endif

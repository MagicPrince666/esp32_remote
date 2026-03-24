#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "driver/uart_vfs.h"
#include "driver/uart.h"

void Serial(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num);

int SendData(const char* data, const int lenght);

int RecvData(char* data, const int lenght);

void AsyncRecv();

#endif

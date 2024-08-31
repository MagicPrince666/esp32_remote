#include "peripherals.h"
#include "serial.h"
#include "rocker.h"

Serial *g_serial = nullptr;
#define TXD1_PIN 18
#define RXD1_PIN 5

Rocker *g_rocker = nullptr;

void InitAll()
{
    g_serial = new Serial(UART_NUM_1, TXD1_PIN, RXD1_PIN);
    g_rocker = new Rocker();
}
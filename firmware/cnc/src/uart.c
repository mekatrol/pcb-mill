#include <stdint.h>
#include "uart.h"

#define PCLK 120000000 // CPU clock frequency (Hz)
#define UART0_BASE 0x4000C000

#define U0THR (*(volatile uint32_t *)(UART0_BASE + 0x00)) // Transmit Holding
#define U0LSR (*(volatile uint32_t *)(UART0_BASE + 0x14)) // Line Status
#define U0LCR (*(volatile uint32_t *)(UART0_BASE + 0x0C)) // Line Control
#define U0DLL (*(volatile uint32_t *)(UART0_BASE + 0x00)) // Divisor Latch LSB
#define U0DLM (*(volatile uint32_t *)(UART0_BASE + 0x04)) // Divisor Latch MSB
#define U0FDR (*(volatile uint32_t *)(UART0_BASE + 0x28)) // Fractional Divider
#define U0FCR (*(volatile uint32_t *)(UART0_BASE + 0x08)) // FIFO Control
#define U0LCR_DLAB (1 << 7)
#define U0LSR_THRE (1 << 5)

void uart_init(uint32_t baudrate)
{
    // Enable UART0 on P0.2 (TXD0) and P0.3 (RXD0)
    volatile uint32_t *PINSEL0 = (uint32_t *)0x4002C000;
    *PINSEL0 &= ~((3 << 4) | (3 << 6)); // Clear bits for P0.2 and P0.3
    *PINSEL0 |= (1 << 4) | (1 << 6);    // Set P0.2 to TXD0, P0.3 to RXD0

    // Enable DLAB to set baud rate
    U0LCR = 0x83; // 8-bit, no parity, 1 stop bit, DLAB=1

    uint32_t divider = PCLK / (16 * baudrate);
    U0DLL = divider & 0xFF;
    U0DLM = (divider >> 8) & 0xFF;

    U0FDR = (1 << 4) | 0; // DIVADDVAL=0, MULVAL=1 (no fractional division)
    U0LCR &= ~U0LCR_DLAB; // Clear DLAB

    U0FCR = 0x07; // Enable FIFO, reset RX/TX
}

void uart_putc(char c)
{
    while (!(U0LSR & U0LSR_THRE))
        ;
    U0THR = c;
}

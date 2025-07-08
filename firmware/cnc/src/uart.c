#include <stdint.h>

#define PCLK_UART0 120000000 // Must match your system clock (F_CPU)

#define PINSEL0 (*(volatile uint32_t *)0x4002C000)
#define U0LCR (*(volatile uint32_t *)0x4000C00C)
#define U0DLL (*(volatile uint32_t *)0x4000C000)
#define U0DLM (*(volatile uint32_t *)0x4000C004)
#define U0FDR (*(volatile uint32_t *)0x4000C028)
#define U0FCR (*(volatile uint32_t *)0x4000C008)
#define U0LSR (*(volatile uint32_t *)0x4000C014)
#define U0THR (*(volatile uint32_t *)0x4000C000)
#define PCLKSEL0 (*(volatile uint32_t *)0x400FC1A8)

void uart_init(uint32_t baud)
{
    // 0. Set PCLK for UART0 = CCLK
    PCLKSEL0 &= ~(3 << 6); // Clear bits
    PCLKSEL0 |= (1 << 6);  // PCLK_UART0 = CCLK (120 MHz)

    // 1. Configure TXD0/RXD0
    PINSEL0 &= ~((3 << 4) | (3 << 6));
    PINSEL0 |= (1 << 4) | (1 << 6);

    // 2. Enable DLAB and set word length
    U0LCR = (1 << 7) | 0x03; // DLAB=1, 8N1

    // 3. Set baud rate divisor
    uint32_t dl = 120000000 / (16 * baud); // PCLK_UART0 = 120 MHz
    U0DLL = dl & 0xFF;
    U0DLM = (dl >> 8) & 0xFF;

    U0FDR = (1 << 4); // No fractional divider

    // 4. Clear DLAB
    U0LCR &= ~(1 << 7);

    // 5. Enable and reset FIFOs
    U0FCR = 0x07;
}

void uart_send_char(char c)
{
    while ((U0LSR & (1 << 5)) == 0)
        ; // Wait for THR empty
    U0THR = c;
}

void uart_send_str(const char *s)
{
    while (*s)
        uart_send_char(*s++);
}

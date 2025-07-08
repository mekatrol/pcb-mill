#include <stdint.h>
#include "systick.h"
#include "uart.h"

#define GPIO2_DIR (*(volatile uint32_t *)0x2009C040)
#define GPIO2_SET (*(volatile uint32_t *)0x2009C058)
#define GPIO2_CLR (*(volatile uint32_t *)0x2009C05C)

#define GPIO2_PINS ((1 << 3) | (1 << 4) | (1 << 5) | (1 << 7))

int main(void)
{
    clock_init();
    systick_init();
    // uart_init(115200); // UART0 @ 115200 baud

    GPIO2_DIR |= GPIO2_PINS;

    while (1)
    {
        GPIO2_SET = GPIO2_PINS;
        delay_ms(1000);

        GPIO2_CLR = GPIO2_PINS;
        delay_ms(1000);
    }

    // Should never return
    return -1;
}

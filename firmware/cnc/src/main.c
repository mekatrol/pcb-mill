#include <stdint.h>
#include "systick.h"
#include "uart.h"

#define GPIO2_DIR (*(volatile uint32_t *)0x2009C040)
#define GPIO2_SET (*(volatile uint32_t *)0x2009C058)
#define GPIO2_CLR (*(volatile uint32_t *)0x2009C05C)

int main(void)
{
    systick_init();
    uart_init(115200); // UART0 @ 115200 baud

    GPIO2_DIR |= (1 << 4);

    while (1)
    {
        GPIO2_SET = (1 << 4);
        delay_ms(1000);
        GPIO2_CLR = (1 << 4);
        delay_ms(1000);
    }
}

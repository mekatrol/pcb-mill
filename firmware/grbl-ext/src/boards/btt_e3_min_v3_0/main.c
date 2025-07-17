#include <stdint.h>
#include <stdbool.h>

#include "clock.h"
#include "fans.h"
#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"
#include "../../drivers/steppers/steppers.h"
#include "../../drivers/timers/timers.h"
#include "../../drivers/uart/uart.h"
#include "../../drivers/tmc2209/tmc2209.h"
#include "../../drivers/uart/uart.h"
#include "../../drivers/logging/log_uart.h"

int main(void)
{
    init_clock();

    // Enable GPIO ports
    RCC->IOPENR |= IOPENR_PORTA_ENABLE; // Enable PORTA
    RCC->IOPENR |= IOPENR_PORTB_ENABLE; // Enable PORTB
    RCC->IOPENR |= IOPENR_PORTC_ENABLE; // Enable PORTC
    RCC->IOPENR |= IOPENR_PORTD_ENABLE; // Enable PORTD

    // Enable GPIOC peripheral
    GPIOC->MODER &= ~(MODER_MSK << (BIT_06 * MODER_BIT_COUNT)); // Clear PC6 mode bits
    GPIOC->MODER |= (MODER_OUT << (BIT_06 * MODER_BIT_COUNT));  // Set PC6 as output

    // Enable GPIOD peripheral
    GPIOD->MODER &= ~(MODER_MSK << (BIT_08 * MODER_BIT_COUNT)); // Clear PD8 mode bits
    GPIOD->MODER |= (MODER_OUT << (BIT_08 * MODER_BIT_COUNT));  // Set PD8 as output

    timer6_init(500, true);
    timer7_init(1000, true);
    timer14_init();

    uart2_init();
    uart4_init();

    uart_puts("Hello!\r\n");

    init_steppers();

    enable_irq();

    delay_ms(100);

    tmc2209_read_gconf(0x00); // Send read to slave 0

    delay_ms(100);

    uint8_t gconf[4];
    uint32_t value;
    int result = tmc2209_parse_reply(4, gconf);
    if (result == 0)
    {
        value = gconf[0] | (gconf[1] << 8) | (gconf[2] << 16) | (gconf[3] << 24);
        uart_printf("gconf: 0x%x\r\n", value);
    }
    else
    {
        // Handle error
        set_timer6_interval(100);
    }

    while (1)
    {
        uint8_t c;
        while ((c = uart_getc()) != 0)
        {
            uart4_send(c);
        }

        while ((c = uart4_recv()) != 0)
        {
            uart_putc(c);
        }
        delay_ms(10);
    }
}

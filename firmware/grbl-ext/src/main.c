#include <stdint.h>
#include <stdbool.h>

#include "clock.h"
#include "fans.h"
#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"
#include "steppers.h"
#include "timers.h"
#include "uart.h"

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
        (void)value; // Just to stop unused variable warning
        // Use 'value' (GCONF register)
    }
    else
    {
        // Handle error
        set_timer6_interval(100);
    }

    while (1)
    {
        uint8_t c;
        while ((c = uart2_recv()) != 0)
        {
            uart4_send(c);
        }

        while ((c = uart4_recv()) != 0)
        {
            uart2_send(c);
        }
        delay_ms(10);
    }
}

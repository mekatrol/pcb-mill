#include <stdint.h>
#include <stdbool.h>

#include "clock.h"
#include "fans.h"
#include "memory_map.h"
#include "register_bits.h"
#include "timers.h"
#include "uart.h"

extern void usart2_init();
extern void usart2_test(void);

int main(void)
{
    init_clock();

    // Enable GPIO ports
    RCC->IOPENR |= IOPENR_PORTA_ENABLE; // Enable PORTA in IOPENR
    RCC->IOPENR |= IOPENR_PORTC_ENABLE; // Enable PORTC in IOPENR
    RCC->IOPENR |= IOPENR_PORTD_ENABLE; // Enable PORTD in IOPENR

    // Enable GPIOC peripheral
    GPIOC->MODER &= ~(MODER_MSK << (BIT_06 * MODER_BIT_COUNT)); // Clear PC6 mode bits
    GPIOC->MODER |= (MODER_OUT << (BIT_06 * MODER_BIT_COUNT));  // Set PC6 as output

    // Enable GPIOD peripheral
    GPIOD->MODER &= ~(MODER_MSK << (BIT_08 * MODER_BIT_COUNT)); // Clear PD8 mode bits
    GPIOD->MODER |= (MODER_OUT << (BIT_08 * MODER_BIT_COUNT));  // Set PD8 as output

    usart2_init();
    usart2_test();

    timer6_init(500, true);
    timer7_init(1000, true);

    uart4_init();

    __asm volatile("cpsie i");

    delay_ms(100);

    tmc2209_read_gconf(0x00); // Send read to slave 0

    delay_ms(1);

    uint8_t gconf[4];
    int result = tmc2209_parse_reply(gconf, 0x00);
    if (result == 0)
    {
        uint32_t value = gconf[0] | (gconf[1] << 8) | (gconf[2] << 16) | (gconf[3] << 24);
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
        delay_ms(5000);
    }
}

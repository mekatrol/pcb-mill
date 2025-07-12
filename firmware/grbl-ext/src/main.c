#include <stdint.h>
#include <stdbool.h>

#include "clock.h"
#include "memory_map.h"
#include "ports.h"
#include "timers.h"

int main(void)
{
    init_clock();

    // Enable GPIOC peripheral
    RCC->IOPENR |= IOPENR_PORTC_ENABLE;                          // Enable PORTC in IOPENR
    GPIOC->MODER &= ~(MODER_MSK << (MODE_06 * MODER_BIT_COUNT)); // Clear PC6 mode bits
    GPIOC->MODER |= (MODER_OUT << (MODE_06 * MODER_BIT_COUNT));  // Set PC6 as output

    // Enable GPIOD peripheral
    RCC->IOPENR |= IOPENR_PORTD_ENABLE;                          // Enable PORTC in IOPENR
    GPIOD->MODER &= ~(MODER_MSK << (MODE_08 * MODER_BIT_COUNT)); // Clear PD8 mode bits
    GPIOD->MODER |= (MODER_OUT << (MODE_08 * MODER_BIT_COUNT));  // Set PD8 as output

    timer6_init(1000, true);
    timer7_init(500, true);

    // Wait 10 seconds
    delay_ms(10000);

    // Update intervals
    set_timer7_interval(1000);

    while (1)
    {
        delay_ms(100);
    }
}

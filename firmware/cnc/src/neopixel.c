#include <stdint.h>
#include <stdbool.h>

#include "registers.h"

#define NEOPIXEL_PIN (1 << 21) // P1.21

// Timing constants in CPU cycles for 120 MHz (adjust if your clock differs)
#define CYCLES_T0H 42 // ~350ns
#define CYCLES_T1H 84 // ~700ns
#define CYCLES_T0L 84 // ~700ns
#define CYCLES_T1L 42 // ~350ns

static inline void delay_cycles(volatile uint32_t cycles)
{
    while (cycles--)
    {
        __asm volatile("nop");

        // Tell compiler not to opimize out (assume this loop might touch memory or have side effects)
        __asm volatile("" ::: "memory");
    }
}

void neopixel_send_bit(bool bit_val)
{
    if (bit_val)
    {
        GPIO1->SET = NEOPIXEL_PIN; // Set pin high
        delay_cycles(CYCLES_T1H);
        GPIO1->CLR = NEOPIXEL_PIN; // Set pin low
        delay_cycles(CYCLES_T1L);
    }
    else
    {
        GPIO1->SET = NEOPIXEL_PIN;
        delay_cycles(CYCLES_T0H);
        GPIO1->CLR = NEOPIXEL_PIN;
        delay_cycles(CYCLES_T0L);
    }
}

void neopixel_send_byte(uint8_t byte)
{
    for (int8_t i = 7; i >= 0; i--)
    {
        neopixel_send_bit((byte >> i) & 1);
    }
}

// Send one RGB color (GRB order)
void __neopixel_send_color(uint8_t r, uint8_t g, uint8_t b)
{
    neopixel_send_byte(g);
    neopixel_send_byte(r);
    neopixel_send_byte(b);
}

// Send one RGB color (GRB order)
void neopixel_send_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t primask;

    // Save current interrupt state
    __asm volatile(
        "mrs %0, primask\n"
        "cpsid i\n"
        : "=r"(primask)::"memory");

    __neopixel_send_color(r, g, b);

    // Restore previous interrupt state
    __asm volatile(
        "msr primask, %0\n" ::"r"(primask) : "memory");

    // Latch delay > 50us, approx 6000 cycles of NOP @ 120MHz
    for (volatile uint32_t i = 0; i < 6000; i++)
    {
        __asm volatile("nop");
    }
}

void neopixel_init(void)
{
    // Configure P1.21 as output
    GPIO1->DIR |= NEOPIXEL_PIN;
    GPIO1->CLR = NEOPIXEL_PIN;
}

void neopixel_send_colors(uint8_t (*colors)[3], uint32_t count)
{
    uint32_t primask;

    // Save current interrupt state
    __asm volatile(
        "mrs %0, primask\n"
        "cpsid i\n"
        : "=r"(primask)::"memory");

    for (uint32_t i = 0; i < count; i++)
    {
        __neopixel_send_color(colors[i][0], colors[i][1], colors[i][2]);
    }

    // Restore previous interrupt state
    __asm volatile(
        "msr primask, %0\n" ::"r"(primask) : "memory");

    // Latch delay > 50us, approx 6000 cycles of NOP @ 120MHz
    for (volatile uint32_t i = 0; i < 6000; i++)
    {
        __asm volatile("nop");
    }
}

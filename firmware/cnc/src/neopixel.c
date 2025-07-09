#include <stdint.h>
#include <stdbool.h>

#include "registers.h"

// GPIO port 1 register offsets (from LPC1769 datasheet)
#define FIODIR_OFFSET 0x020 // Direction register
#define FIOSET_OFFSET 0x038 // Set bits register
#define FIOCLR_OFFSET 0x03C // Clear bits register

#define GPIO1_FIODIR (*(volatile uint32_t *)(GPIO1_BASE + FIODIR_OFFSET))
#define GPIO1_FIOSET (*(volatile uint32_t *)(GPIO1_BASE + FIOSET_OFFSET))
#define GPIO1_FIOCLR (*(volatile uint32_t *)(GPIO1_BASE + FIOCLR_OFFSET))

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
    }
}

void neopixel_send_bit(bool bit_val)
{
    if (bit_val)
    {
        GPIO1_FIOSET = NEOPIXEL_PIN; // Set pin high
        delay_cycles(CYCLES_T1H);
        GPIO1_FIOCLR = NEOPIXEL_PIN; // Set pin low
        delay_cycles(CYCLES_T1L);
    }
    else
    {
        GPIO1_FIOSET = NEOPIXEL_PIN;
        delay_cycles(CYCLES_T0H);
        GPIO1_FIOCLR = NEOPIXEL_PIN;
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
void neopixel_send_color(uint8_t r, uint8_t g, uint8_t b)
{
    neopixel_send_byte(g);
    neopixel_send_byte(r);
    neopixel_send_byte(b);
}

void neopixel_init(void)
{
    // Configure P1.21 as output
    GPIO1_FIODIR |= NEOPIXEL_PIN;
    GPIO1_FIOCLR = NEOPIXEL_PIN;
}

void neopixel_send_colors(uint8_t (*colors)[3], uint32_t count)
{
    __asm volatile("cpsid i"); // Disable interrupts

    for (uint32_t i = 0; i < count; i++)
    {
        neopixel_send_color(colors[i][0], colors[i][1], colors[i][2]);
    }

    __asm volatile("cpsie i"); // Enable interrupts

    // Latch delay > 50us, approx 6000 cycles of NOP @ 120MHz
    for (volatile uint32_t i = 0; i < 6000; i++)
    {
        __asm volatile("nop");
    }
}

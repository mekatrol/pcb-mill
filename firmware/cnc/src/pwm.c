#include <stdint.h>

#define PINSEL3 (*(volatile uint32_t *)0x4002C00C)
#define PWM1MR0 (*(volatile uint32_t *)0x40018018)
#define PWM1MR1 (*(volatile uint32_t *)0x4001801C)
#define PWM1MR2 (*(volatile uint32_t *)0x40018020)
#define PWM1MR3 (*(volatile uint32_t *)0x40018024)
#define PWM1LER (*(volatile uint32_t *)0x40018050)
#define PWM1PCR (*(volatile uint32_t *)0x4001804C)
#define PWM1TCR (*(volatile uint32_t *)0x40018004)
#define PWM1PR (*(volatile uint32_t *)0x4001800C)
#define PWM1MCR (*(volatile uint32_t *)0x40018014)
#define PWM1PCR (*(volatile uint32_t *)0x4001804C)
#define PWM1TC (*(volatile uint32_t *)0x40018008)

void pwm_init(void)
{
    // Configure pins to PWM function
    // PWM1.1 = P1.18 (Blue)
    // PWM1.2 = P1.20 (Green)
    // PWM1.3 = P1.21 (Red)
    PINSEL3 |= (1 << 4);  // P1.18: bits 5:4 = 01 (PWM1.1)
    PINSEL3 |= (1 << 12); // P1.20: bits 13:12 = 01 (PWM1.2)
    PINSEL3 |= (1 << 14); // P1.21: bits 15:14 = 01 (PWM1.3)

    PWM1TCR = 0x02; // Reset counter
    PWM1PR = 0;     // No prescale

    PWM1MR0 = 1000; // PWM period
    PWM1MR1 = 0;    // PWM1.1 (Blue)
    PWM1MR2 = 0;    // PWM1.2 (Green)
    PWM1MR3 = 0;    // PWM1.3 (Red)

    PWM1MCR = (1 << 1);                         // Reset on MR0
    PWM1PCR = (1 << 9) | (1 << 10) | (1 << 11); // Enable PWM1.1, 1.2, 1.3 outputs
    PWM1LER = 0x0F;                             // Load MR0-3

    PWM1TCR = 0x09; // Enable counter and PWM
}

void pwm_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    PWM1MR1 = (b * 1000) / 255;               // Blue - PWM1.1 - P1.18
    PWM1MR2 = (g * 1000) / 255;               // Green - PWM1.2 - P1.20
    PWM1MR3 = (r * 1000) / 255;               // Red - PWM1.3 - P1.21
    PWM1LER = (1 << 1) | (1 << 2) | (1 << 3); // Load MR1-3
}

#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <stdint.h>

void neopixel_init(void);
void neopixel_send_color(uint8_t r, uint8_t g, uint8_t b);
void neopixel_send_colors(uint8_t (*colors)[3], uint32_t count);

#endif // NEOPIXEL_H

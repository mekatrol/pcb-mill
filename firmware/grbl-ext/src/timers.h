#include <stdint.h>
#include <stdbool.h>

#ifndef __TIMERS_H__
#define __TIMERS_H__

void delay_ms(uint32_t ms);

void timer6_init(uint32_t interval, bool enable_interrupt);
void timer7_init(uint32_t interval, bool enable_interrupt);
void timer14_init();

void set_timer6_interval(uint32_t interval);
void set_timer7_interval(uint32_t interval);
void set_timer14_interval(uint32_t interval);

#endif // __TIMERS_H__
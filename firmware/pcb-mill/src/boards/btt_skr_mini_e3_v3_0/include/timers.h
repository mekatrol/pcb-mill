#include <stdbool.h>
#include <stdint.h>

#ifndef __TIMERS_H__
#define __TIMERS_H__

void timer6_init();
void status_timer7_init(uint32_t interval, bool enable_interrupt);
void stepper_timer14_init();

void set_timer7_interval(uint32_t interval);

#endif  // __TIMERS_H__
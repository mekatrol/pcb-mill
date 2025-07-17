#include <stdint.h>
#include <stdbool.h>

#include "memory_map.h"

#ifndef __FANS_H__
#define __FANS_H__

void enable_fan_0(uint32_t frequency, uint32_t duty_cycle);
void set_fan_0_pwm(uint32_t frequency, uint32_t duty_cycle);

#endif // __FANS_H__
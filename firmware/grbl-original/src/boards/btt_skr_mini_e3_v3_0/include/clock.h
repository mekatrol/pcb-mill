#include <stdbool.h>
#include <stdint.h>

#ifndef __CLOCK_H__
#define __CLOCK_H__

#define F_SYS_CLOCK 64000000UL

void init_clock();
uint32_t get_systick();
uint32_t get_second_counter();

#endif  // __CLOCK_H__
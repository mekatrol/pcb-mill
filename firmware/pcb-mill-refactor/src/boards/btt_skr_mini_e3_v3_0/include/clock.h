#include <stdbool.h>
#include <stdint.h>

#ifndef __CLOCK_H__
#define __CLOCK_H__

#define F_SYS_CLOCK 64000000UL

void clock_init();
uint32_t get_sys_tick();
uint32_t get_elapsed_seconds();

#endif  // __CLOCK_H__
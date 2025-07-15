#include <stdint.h>
#include <stdbool.h>

#ifndef __TIMERS_H__
#define __TIMERS_H__

#define TIM_CR1_CEN (1 << 0)  //
#define TIM_CR1_OPM (1 << 3)  // One-pulse mode
#define TIM_EGR_UG (1 << 0)   //
#define TIM_SR_UIF (1 << 0)   //
#define TIM_DIER_UIE (1 << 0) //

void delay_ms(uint32_t ms);

void timer6_init(uint32_t interval, bool enable_interrupt);
void timer7_init(uint32_t interval, bool enable_interrupt);

void set_timer6_interval(uint32_t interval);
void set_timer7_interval(uint32_t interval);

#endif // __TIMERS_H__
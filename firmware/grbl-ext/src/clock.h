#include <stdint.h>
#include <stdbool.h>

#ifndef __CLOCK_H__
#define __CLOCK_H__

#define F_CPU 64000000UL

#define RCC_APBENR1_TIM2EN (1 << 0)
#define RCC_APBENR1_TIM3EN (1 << 1)
#define RCC_APBENR1_TIM4EN (1 << 2)
#define RCC_APB1ENR_TIM6EN (1 << 4)
#define RCC_APB1ENR_TIM7EN (1 << 5)
#define RCC_APBENR1_USART2EN (1 << 17)
#define RCC_APBENR1_USART3EN (1 << 18)
#define RCC_APBENR1_USART4EN (1 << 19)

void init_clock();

#endif // __CLOCK_H__
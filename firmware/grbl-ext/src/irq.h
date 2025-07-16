#ifndef __IRQ_H__
#define __IRQ_H__

#define TIM6_IRQn 17
#define TIM7_IRQn 18
#define TIM14_IRQn 19
#define USART3_4_LPUART1_IRQn 29

#define ENABLE_IRQ(irq_number) NVIC->ISER[(irq_number) / 32] |= (1 << ((irq_number) % 32));

#endif // __IRQ_H__
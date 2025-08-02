.syntax       unified
.cpu          cortex-m0plus
.thumb

.global       g_pfnVectors
.global       Reset_Handler
.global       SysTick_IRQHandler
.global       EXTI0_1_IRQHandler
.global       EXTI2_3_IRQHandler
.global       USB_UCPD1_2_IRQHandler
.global       TIM6_DAC_IRQHandler
.global       TIM7_IRQHandler
.global       TIM14_IRQHandler
.global       USART2_IRQHandler
.global       USART3_4_LPUART1_IRQHandler
.global       Default_Handler

.section      .isr_vector, "a", %progbits
.align        2
.type         g_pfnVectors, %object

/***********************************************************************************************************************************************
 * NVIC - Nested vectored interrupt controller
 +----------+----------+-----------------+---------------------------+---------------------------------------------------------------+---------+
 | Position | Priority | Type of Priority| Acronym                   | Description                                                   | Address |
 +----------+----------+-----------------+---------------------------+---------------------------------------------------------------+---------+
 | 0        | 3        | Interrupt       | WWDG                      | Window watchdog                                               | 0x0004  |
 | 1        | 3        | Interrupt       | PVD                       | PVD through EXTI line                                         | 0x0008  |
 | 2        | 3        | Interrupt       | RTC                       | RTC/TAMP                                                      | 0x000C  |
 | 3        | 3        | Interrupt       | FLASH                     | Flash                                                         | 0x0010  |
 | 4        | 3        | Interrupt       | RCC                       | Reset and clock control                                       | 0x0014  |
 | 5        | 3        | Interrupt       | EXTI0_1                   | EXTI Line 0 and 1 interrupts                                  | 0x0018  |
 | 6        | 3        | Interrupt       | EXTI2_3                   | EXTI Line 2 and 3 interrupts                                  | 0x001C  |
 | 7        | 3        | Interrupt       | EXTI4_15                  | EXTI Line 4 to 15 interrupts                                  | 0x0020  |
 | 8        | 3        | Interrupt       | UCPD1                     | USB Type-C Power Delivery                                     | 0x0024  |
 | 9        | 3        | Interrupt       | DMA1_Ch1                  | DMA1 Channel 1                                                | 0x0028  |
 | 10       | 3        | Interrupt       | DMA1_Ch2_3                | DMA1 Channels 2 and 3                                         | 0x002C  |
 | 11       | 3        | Interrupt       | DMA1_Ch4_5_6_7_DMA2_Ch1_2 | DMA1 Ch 4-7, DMA2 Ch 1-2                                      | 0x0030  |
 | 12       | 3        | Interrupt       | ADC_COMP                  | ADC and Comparator                                            | 0x0034  |
 | 13       | 3        | Interrupt       | TIM1_BRK_UP_TRG_COM       | TIM1 Break, Update, Trigger and Commutation interrupts        | 0x0038  |
 | 14       | 3        | Interrupt       | TIM1_CC                   | TIM1 Capture Compare                                          | 0x003C  |
 | 15       | 3        | Interrupt       | TIM2                      | TIM2                                                          | 0x0040  |
 | 16       | 3        | Interrupt       | TIM3                      | TIM3                                                          | 0x0044  |
 | 17       | 3        | Interrupt       | TIM6_DAC_LPTIM1           | TIM6, DAC, LPTIM1 interrupts                                  | 0x0048  |
 | 18       | 3        | Interrupt       | TIM7_LPTIM2               | TIM7, LPTIM2 interrupts                                       | 0x004C  |
 | 19       | 3        | Interrupt       | TIM14                     | TIM14                                                         | 0x0050  |
 | 20       | 3        | Interrupt       | TIM15                     | TIM15                                                         | 0x0054  |
 | 21       | 3        | Interrupt       | TIM16                     | TIM16                                                         | 0x0058  |
 | 22       | 3        | Interrupt       | TIM17                     | TIM17                                                         | 0x005C  |
 | 23       | 3        | Interrupt       | I2C1                      | I2C1                                                          | 0x0060  |
 | 24       | 3        | Interrupt       | I2C2                      | I2C2                                                          | 0x0064  |
 | 25       | 3        | Interrupt       | SPI1                      | SPI1                                                          | 0x0068  |
 | 26       | 3        | Interrupt       | SPI2                      | SPI2                                                          | 0x006C  |
 | 27       | 3        | Interrupt       | USART1                    | USART1                                                        | 0x0070  |
 | 28       | 3        | Interrupt       | USART2                    | USART2                                                        | 0x0074  |
 | 29       | 3        | Interrupt       | USART3_USART4_LPUART1     | USART3/4 and LPUART1                                          | 0x0078  |
 | 30       | 3        | Interrupt       | CEC_CAN                   | CEC and CAN                                                   | 0x007C  |
 | 31       | 3        | Interrupt       | USB_UCPD2                 | USB and UCPD2                                                 | 0x0080  |
 +----------+----------+-----------------+---------------------------+---------------------------------------------------------------+---------+*/

g_pfnVectors:
.word         _estack                       // Initial stack pointer
.word         Reset_Handler                 // Reset
.word         NMI_Handler
.word         HardFault_Handler
.word         0, 0, 0, 0                    // Reserved
.word         0, 0, 0                       // Reserved
.word         SVC_Handler
.word         0                             // Reserved
.word         0                             // Reserved
.word         PendSV_Handler
.word         SysTick_Handler

  /* IRQs: STM32G0B1 has 48 IRQs */
.word         Default_Handler               // IRQ 0
.word         Default_Handler               // IRQ 1
.word         Default_Handler               // IRQ 2
.word         Default_Handler               // IRQ 3
.word         Default_Handler               // IRQ 4
.word         EXTI0_1_IRQHandler            // IRQ 5
.word         EXTI2_3_IRQHandler            // IRQ 6
.word         Default_Handler               // IRQ 7
.word         USB_UCPD1_2_IRQHandler        // IRQ 8
.word         Default_Handler               // IRQ 9
.word         Default_Handler               // IRQ10
.word         Default_Handler               // IRQ11
.word         Default_Handler               // IRQ12
.word         Default_Handler               // IRQ13
.word         Default_Handler               // IRQ14
.word         Default_Handler               // IRQ15
.word         Default_Handler               // IRQ16
.word         TIM6_DAC_IRQHandler           // IRQ17 = TIM6/DAC
.word         TIM7_IRQHandler               // IRQ18 = TIM7
.word         TIM14_IRQHandler              // IRQ19
.word         Default_Handler               // IRQ20
.word         Default_Handler               // IRQ21
.word         Default_Handler               // IRQ22
.word         Default_Handler               // IRQ23
.word         Default_Handler               // IRQ24
.word         Default_Handler               // IRQ25
.word         Default_Handler               // IRQ26
.word         Default_Handler               // IRQ27
.word         USART2_IRQHandler             // IRQ28
.word         USART3_4_LPUART1_IRQHandler   // IRQ29
.word         Default_Handler               // IRQ30
.word         Default_Handler               // IRQ31

.size         g_pfnVectors, . - g_pfnVectors

.global       main
.type         main, %function

// Reset Handler
.section      .text.Reset_Handler
.type         Reset_Handler, %function
Reset_Handler:
   // Zero BSS
   ldr        r0, =_sbss
   ldr        r1, =_ebss
   movs       r2, #0

zero_bss:
   cmp        r0, r1
   bcc        1f
   b          zero_done
1:
   str        r2, [r0]
   adds       r0, r0, #4
   b          zero_bss

zero_done:

   // Copy .data section from FLASH to RAM
   ldr        r0, =_sdata                   // dest
   ldr        r1, =_edata                   // end
   ldr        r2, =_sidata                  // source

copy_data:
   cmp        r0, r1
   bcc        copy_more
   b          call_main

copy_more:
   ldr        r3, [r2]
   str        r3, [r0]
   adds       r0, r0, #4
   adds       r2, r2, #4
   b          copy_data


call_main:
   bl         main
   b          .

// Default Handlers
Default_Handler:
   b          .

// Hard fault Handler
HardFault_Handler:
   b          .

   NMI_Handler = Default_Handler
   SVC_Handler = Default_Handler
   PendSV_Handler = Default_Handler

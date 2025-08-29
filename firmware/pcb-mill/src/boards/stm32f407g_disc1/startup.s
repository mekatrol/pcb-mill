.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.global _start
.section .isr_vector,"a",%progbits
.type _start, %function

_start:
.word  _estack
.word  Reset_Handler
.word  0   /* NMI_Handler */
.word  0   /* HardFault_Handler */
.word  0   /* MemManage_Handler */
.word  0   /* BusFault_Handler */
.word  0   /* UsageFault_Handler */
.word  0   /* Reserved */
.word  0   /* Reserved */
.word  0   /* Reserved */
.word  0   /* Reserved */
.word  0   /* SVC_Handler */
.word  0   /* DebugMon_Handler */
.word  0   /* Reserved */
.word  0   /* PendSV_Handler */
.word  0   /* SysTick_Handler */

.section .text.Reset_Handler
Reset_Handler:
  ldr   sp, =_estack
  bl    main
  b     .
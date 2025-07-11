.syntax unified
.cpu cortex-m0plus
.thumb

.global g_pfnVectors
.global Reset_Handler
.global Default_Handler

.section .isr_vector, "a", %progbits
.align  2
.type g_pfnVectors, %object

g_pfnVectors:
  .word _estack            // Initial stack pointer
  .word Reset_Handler      // Reset
  .word NMI_Handler
  .word HardFault_Handler
  .word 0, 0, 0, 0         // Reserved
  .word 0, 0, 0            // Reserved
  .word SVC_Handler
  .word 0                  // Reserved
  .word 0                  // Reserved
  .word PendSV_Handler
  .word SysTick_Handler

  /* IRQs: STM32G0B1 has 48 IRQs */
  .rept 48
  .word Default_Handler
  .endr

.size g_pfnVectors, . - g_pfnVectors

.global main
.type main, %function

// Reset Handler
.section .text.Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
  // Zero BSS
  ldr   r0, =_sbss
  ldr   r1, =_ebss
  movs  r2, #0

zero_bss:
  cmp   r0, r1
  bcc   1f
  b     zero_done
1:
  str   r2, [r0]
  adds  r0, r0, #4
  b     zero_bss

zero_done:

  // Copy .data section from FLASH to RAM
  ldr r0, =_sdata     // dest
  ldr r1, =_edata     // end
  ldr r2, =_sidata    // source

copy_data:
  cmp r0, r1
  bcc copy_more
  b call_main

copy_more:
  ldr r3, [r2]
  str r3, [r0]
  adds r0, r0, #4
  adds r2, r2, #4
  b copy_data


call_main:
  bl main
  b .

// Default Handlers
Default_Handler:
  b .

NMI_Handler       = Default_Handler
HardFault_Handler = Default_Handler
SVC_Handler       = Default_Handler
PendSV_Handler    = Default_Handler
SysTick_Handler   = Default_Handler



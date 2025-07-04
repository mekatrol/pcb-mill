#include <stdint.h>

extern int main(void);

/* Symbols provided by the linker script */
extern uint32_t _etext;  // End of .text (start of initialized data values in flash)
extern uint32_t _sdata;  // Start of .data in RAM
extern uint32_t _edata;  // End of .data in RAM
extern uint32_t _sbss;   // Start of .bss in RAM
extern uint32_t _ebss;   // End of .bss in RAM
extern uint32_t _estack; // Top of stack (from linker script)

/* Function prototypes */
__attribute__((noreturn)) void Reset_Handler(void);
__attribute__((noreturn)) void Default_Handler(void);

/* Vector table */
__attribute__((section(".isr_vector"), used))
const void *vector_table[] = {
    (void *)&_estack, // Initial stack pointer
    Reset_Handler,    // Reset
    Default_Handler,  // NMI
    Default_Handler,  // Hard fault
    Default_Handler,  // MemManage
    Default_Handler,  // BusFault
    Default_Handler,  // UsageFault
    0, 0, 0, 0,       // Reserved
    Default_Handler,  // SVCall
    Default_Handler,  // Debug monitor
    0,                // Reserved
    Default_Handler,  // PendSV
    Default_Handler,  // SysTick
    // You can add IRQs here if needed
};

/* Reset Handler */
void Reset_Handler(void)
{
    // Copy initialized data from flash to RAM
    uint32_t *src = (uint32_t *)&_etext;
    uint32_t *dst = (uint32_t *)&_sdata;
    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    // Zero initialize BSS section
    dst = &_sbss;
    while (dst < &_ebss)
    {
        *dst++ = 0;
    }

    // Call main()
    main();

    // If main returns, loop forever
    while (1)
        ;
}

/* Default Handler for unimplemented interrupts */
void Default_Handler(void)
{
    while (1)
        ;
}

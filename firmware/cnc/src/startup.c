#include <stdint.h>

/*
Vector Table Layout (LPC1769 / Cortex-M3)
Offset	Vector Name	Description	Notes / Default Handler
0x00	Initial Stack Pointer	Top of RAM (e.g. 0x10010000)	MSP loaded on reset
0x04	Reset_Handler	Entry point after reset	Your Reset_Handler()
0x08	NMI_Handler	Non-Maskable Interrupt	Optional
0x0C	HardFault_Handler	On hard faults	Must handle or reset
0x10	MemManage_Handler	MPU memory protection fault	Optional
0x14	BusFault_Handler	Bus error	Optional
0x18	UsageFault_Handler	Undefined instruction / divide by 0	Optional
0x1C	Reserved	Reserved	Must be 0
0x20	Reserved	Reserved	Must be 0
0x24	Reserved	Reserved	Must be 0
0x28	Reserved	Reserved	Must be 0
0x2C	SVC_Handler	Supervisor call	Optional
0x30	DebugMon_Handler	Debug monitor	Optional
0x34	Reserved	Reserved	Must be 0
0x38	PendSV_Handler	Pendable request for system service	Used in RTOS
0x3C	SysTick_Handler	SysTick timer interrupt	Used in RTOS or SysTick timer

LPC1769 External Interrupt Vector Table
Vector Index	Offset	IRQ #	Handler Name	Description
16	0x40	0	WDT_IRQHandler	Watchdog Timer
17	0x44	1	TIMER0_IRQHandler	Timer 0 Match
18	0x48	2	TIMER1_IRQHandler	Timer 1 Match
19	0x4C	3	TIMER2_IRQHandler	Timer 2 Match
20	0x50	4	TIMER3_IRQHandler	Timer 3 Match
21	0x54	5	UART0_IRQHandler	UART0 Rx/Tx
22	0x58	6	UART1_IRQHandler	UART1 Rx/Tx
23	0x5C	7	UART2_IRQHandler	UART2 Rx/Tx
24	0x60	8	UART3_IRQHandler	UART3 Rx/Tx
25	0x64	9	PWM1_IRQHandler	PWM1 Capture and Match
26	0x68	10	I2C0_IRQHandler	I2C0 Transfer Complete
27	0x6C	11	I2C1_IRQHandler	I2C1 Transfer Complete
28	0x70	12	I2C2_IRQHandler	I2C2 Transfer Complete
29	0x74	13	SPI_IRQHandler	SPI Interrupt (not SSP0/1)
30	0x78	14	SSP0_IRQHandler	SSP0 Rx/Tx
31	0x7C	15	SSP1_IRQHandler	SSP1 Rx/Tx
32	0x80	16	PLL0_IRQHandler	PLL0 Lock
33	0x84	17	RTC_IRQHandler	Real-Time Clock
34	0x88	18	EINT0_IRQHandler	External Interrupt 0
35	0x8C	19	EINT1_IRQHandler	External Interrupt 1
36	0x90	20	EINT2_IRQHandler	External Interrupt 2
37	0x94	21	EINT3_IRQHandler	External Interrupt 3
38	0x98	22	ADC_IRQHandler	A/D Converter
39	0x9C	23	BOD_IRQHandler	Brown-Out Detection
40	0xA0	24	USB_IRQHandler	USB Interrupt
41	0xA4	25	CAN_IRQHandler	CAN Rx/Tx
42	0xA8	26	DMA_IRQHandler	GPDMA Transfer
43	0xAC	27	I2S_IRQHandler	I2S Rx/Tx
44	0xB0	28	ENET_IRQHandler	Ethernet
45	0xB4	29	RIT_IRQHandler	Repetitive Interrupt Timer
46	0xB8	30	MCPWM_IRQHandler	Motor Control PWM
47	0xBC	31	QEI_IRQHandler	Quadrature Encoder Interface

*/

int main(void);

/* Symbols provided by the linker script */
extern uint32_t _etext;  // End of .text (start of initialized data values in flash)
extern uint32_t _sdata;  // Start of .data in RAM
extern uint32_t _edata;  // End of .data in RAM
extern uint32_t _sbss;   // Start of .bss in RAM
extern uint32_t _ebss;   // End of .bss in RAM
extern uint32_t _estack; // Top of stack (from linker script)

/* Default handler can be accessed via weak alias */
__attribute__((noreturn)) void Default_Handler(void);

__attribute__((noreturn)) void Reset_Handler(void);
__attribute__((noreturn)) void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void HardFault_Handler(void);
__attribute__((noreturn)) void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void BusFault_Handler(void);
__attribute__((noreturn)) void UsageFault_Handler(void);
__attribute__((noreturn)) void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void WDT_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void TIMER0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void TIMER1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void TIMER2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void TIMER3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void UART0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void UART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void UART2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void UART3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void PWM1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void I2C0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void I2C1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void I2C2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void SPI_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void SSP0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void SSP1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void PLL0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void RTC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void EINT0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void EINT1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void EINT2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void EINT3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void ADC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void BOD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void USB_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void CAN_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void DMA_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void I2S_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void ENET_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void RIT_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void MCPWM_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((noreturn)) void QEI_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

/* Vector table for LPC1769 */
__attribute__((section(".isr_vector"), used))
const uint32_t vector_table[] = {
    (uint32_t)&_estack,      // Initial stack pointer
    (uint32_t)Reset_Handler, // Reset handler
    (uint32_t)NMI_Handler,
    (uint32_t)HardFault_Handler,
    (uint32_t)MemManage_Handler,
    (uint32_t)BusFault_Handler,
    (uint32_t)UsageFault_Handler,
    0, 0, 0, 0, // Reserved
    (uint32_t)SVC_Handler,
    (uint32_t)DebugMon_Handler,
    0,
    (uint32_t)PendSV_Handler,
    (uint32_t)SysTick_Handler,

    // External interrupts (IRQ0 to IRQ31)
    (uint32_t)WDT_IRQHandler,
    (uint32_t)TIMER0_IRQHandler,
    (uint32_t)TIMER1_IRQHandler,
    (uint32_t)TIMER2_IRQHandler,
    (uint32_t)TIMER3_IRQHandler,
    (uint32_t)UART0_IRQHandler,
    (uint32_t)UART1_IRQHandler,
    (uint32_t)UART2_IRQHandler,
    (uint32_t)UART3_IRQHandler,
    (uint32_t)PWM1_IRQHandler,
    (uint32_t)I2C0_IRQHandler,
    (uint32_t)I2C1_IRQHandler,
    (uint32_t)I2C2_IRQHandler,
    (uint32_t)SPI_IRQHandler,
    (uint32_t)SSP0_IRQHandler,
    (uint32_t)SSP1_IRQHandler,
    (uint32_t)PLL0_IRQHandler,
    (uint32_t)RTC_IRQHandler,
    (uint32_t)EINT0_IRQHandler,
    (uint32_t)EINT1_IRQHandler,
    (uint32_t)EINT2_IRQHandler,
    (uint32_t)EINT3_IRQHandler,
    (uint32_t)ADC_IRQHandler,
    (uint32_t)BOD_IRQHandler,
    (uint32_t)USB_IRQHandler,
    (uint32_t)CAN_IRQHandler,
    (uint32_t)DMA_IRQHandler,
    (uint32_t)I2S_IRQHandler,
    (uint32_t)ENET_IRQHandler,
    (uint32_t)RIT_IRQHandler,
    (uint32_t)MCPWM_IRQHandler,
    (uint32_t)QEI_IRQHandler};

#define LPC_WDT_BASE 0x40000000UL

typedef struct
{
    volatile uint32_t WDMOD;    // 0x00 Watchdog mode register
    volatile uint32_t WDTC;     // 0x04 Watchdog timer constant
    volatile uint32_t WDFEED;   // 0x08 Watchdog feed register
    volatile uint32_t WDTV;     // 0x0C Watchdog timer value
    volatile uint32_t WDCLKSEL; // 0x10 Watchdog clock select register
} LPC_WDT_TypeDef;

#define LPC_WDT ((LPC_WDT_TypeDef *)LPC_WDT_BASE)
#define GPIO2_DIR (*(volatile uint32_t *)0x2009C040)
#define GPIO2_SET (*(volatile uint32_t *)0x2009C058)
#define GPIO2_CLR (*(volatile uint32_t *)0x2009C05C)

/* Reset Handler */
void Reset_Handler(void)
{
    // Disable watchdog early
    LPC_WDT->WDCLKSEL = 0x00;   // Select internal RC
    LPC_WDT->WDTC = 0xFFFFFFFF; // Max timeout
    LPC_WDT->WDMOD = 0x00;      // Disable watchdog

    // Copy initialised data from flash to RAM
    uint32_t *src = (uint32_t *)&_etext;
    uint32_t *dst = (uint32_t *)&_sdata;
    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    // Zero initialise BSS section
    dst = (uint32_t *)&_sbss;
    while (dst < (uint32_t *)&_ebss)
    {
        *dst++ = 0;
    }

    // Call main()
    main();

    // If main returns, loop forever
    while (1)
        ;
}

static inline uint32_t get_ipsr(void)
{
    uint32_t result;
    __asm volatile("mrs %0, ipsr" : "=r"(result));
    return result;
}

#define SCB_AIRCR (*((volatile uint32_t *)0xE000ED0C))
#define AIRCR_VECTKEY (0x5FA << 16)
#define AIRCR_SYSRESETREQ (1 << 2)

static inline void NVIC_SystemReset(void)
{
    SCB_AIRCR = AIRCR_VECTKEY | AIRCR_SYSRESETREQ;

    // Wait for reset to take effect
    while (1)
        ;
}

/* Default Handler for unimplemented interrupts */
void Default_Handler(void)
{
    // Should never happen
    while (1)
        ;
}

__attribute__((noreturn)) void HardFault_Handler(void)
{
    uint32_t ipsr = get_ipsr();
    uint32_t irqn = (ipsr >= 16) ? (ipsr - 16) : 0xFFFFFFFF;

    // Insert optional error handling/logging here

    // Disable interrupts
    __asm volatile("cpsid i");

    // Reset system to recover safely
    NVIC_SystemReset();

    // Should never happen
    while (1)
        ;
}

__attribute__((noreturn)) void BusFault_Handler(void)
{
    uint32_t ipsr = get_ipsr();
    uint32_t irqn = (ipsr >= 16) ? (ipsr - 16) : 0xFFFFFFFF;

    // Insert optional error handling/logging here

    // Disable interrupts
    __asm volatile("cpsid i");

    // Reset system to recover safely
    NVIC_SystemReset();

    // Should never happen
    while (1)
        ;
}

__attribute__((noreturn)) void UsageFault_Handler(void)
{
    uint32_t ipsr = get_ipsr();
    uint32_t irqn = (ipsr >= 16) ? (ipsr - 16) : 0xFFFFFFFF;

    // Insert optional error handling/logging here

    // Disable interrupts
    __asm volatile("cpsid i");

    // Reset system to recover safely
    NVIC_SystemReset();

    // Should never happen
    while (1)
        ;
}

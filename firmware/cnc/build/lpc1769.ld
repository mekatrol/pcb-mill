/* Linker script for LPC1769 (256 KB Flash, 64 KB RAM_LPC) */
ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
  RAM_LPC  (rw)  : ORIGIN = 0x10000000, LENGTH = 32K  /* main .data/.bss */
  RAM_AHB0 (rw)  : ORIGIN = 0x2007C000, LENGTH = 16K  /* optional stack */
  RAM_AHB1 (rw)  : ORIGIN = 0x20080000, LENGTH = 16K  /* optional heap/buffers */
}

SECTIONS
{
  /*──────────────────────────────────────────────────────────────────────
    .isr_vector in FLASH
    ──────────────────────────────────────────────────────────────────────*/
  .isr_vector ALIGN(4) : {
    KEEP(*(.isr_vector))
  } > FLASH

  /*──────────────────────────────────────────────────────────────────────
    .text in FLASH
    ──────────────────────────────────────────────────────────────────────*/
  .text ALIGN(4) : {
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;              /* record end‐of‐flash load address */
  } > FLASH

  /*──────────────────────────────────────────────────────────────────────
    .data: VMA in RAM_LPC, LMA at _etext in FLASH
    ──────────────────────────────────────────────────────────────────────*/
  . = ALIGN(4);
  . = ORIGIN(RAM_LPC);
  .data ALIGN(4) : AT(_etext) {
    _sdata = .;              /* runtime start in RAM_LPC */
    *(.data*)
    _edata = .;              /* runtime end in RAM_LPC */
  } > RAM_LPC

  /*──────────────────────────────────────────────────────────────────────
    .bss: in RAM_LPC, immediately after .data
    ──────────────────────────────────────────────────────────────────────*/
  . = ALIGN(4);
  .bss ALIGN(4) : {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
    _end  = .;
  } > RAM_LPC

  .stack (COPY):
  {
    . = ALIGN(8);
    _sstack = .;
    . = . + 0x1000;  /* 4 KB stack */
    _estack_actual = .;
    _estack = .;
  } > RAM_AHB0

  /* Optionally define heap in AHB1 */
  .heap (COPY):
  {
    . = ALIGN(8);
    _sheap = .;
    . = . + 0x1000;  /* 4 KB heap */
    _eheap = .;
  } > RAM_AHB1
}

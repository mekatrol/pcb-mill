/* Linker script for LPC1769 (256 KB Flash, 64 KB RAM) */
ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
  RAM   (rwx) : ORIGIN = 0x10000000, LENGTH = 64K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  /*──────────────────────────────────────────────────────────────────────
    .text in FLASH
    ──────────────────────────────────────────────────────────────────────*/
  .text ALIGN(4) : {
    KEEP(*(.isr_vector))
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;              /* record end‐of‐flash load address */
  } > FLASH

  /*──────────────────────────────────────────────────────────────────────
    .data: VMA in RAM, LMA at _etext in FLASH
    ──────────────────────────────────────────────────────────────────────*/
  . = ALIGN(4);
  . = ORIGIN(RAM);
  .data ALIGN(4) : AT(_etext) {
    _sdata = .;              /* runtime start in RAM */
    *(.data*)
    _edata = .;              /* runtime end in RAM */
  } > RAM

  /*──────────────────────────────────────────────────────────────────────
    .bss: in RAM, immediately after .data
    ──────────────────────────────────────────────────────────────────────*/
  . = ALIGN(4);
  .bss ALIGN(4) : {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
    _end  = .;
  } > RAM
}

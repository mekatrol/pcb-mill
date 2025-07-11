ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 144K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

_sidata = LOADADDR(.data);
_sdata = .;
_edata = .;
_sbss = .;
_ebss = .;


SECTIONS
{
  .isr_vector : {
    KEEP(*(.isr_vector))
  } > FLASH

  .text : {
    *(.text*)
    *(.rodata*)
  } > FLASH

  .data : {
    _sdata = .;
    _sidata = LOADADDR(.data);
    *(.data*)
    _edata = .;
  } > RAM AT > FLASH

  .bss : {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > RAM
}

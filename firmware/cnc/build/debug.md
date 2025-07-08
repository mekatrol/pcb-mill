# Debugging firmware

# openocd general

1. `make load` to flash and launch OCD listending for gdb on port 3333
2. `arm-none-eabi-gdb`
3. `target extended-remote :3333` to connect to openocd
4. `monitor reset init` to initialise device state
5. `monitor flash write_image erase firmware/firmware.bin` to flash the firmware
6. `monitor reset halt` reset board and halt before executing boot
7. `monitor resume` to resume running program
8. `monitor halt` to stop execution
9. `monitor reset` to reset the board and run code

# gdb

> `file firmware/firmware.bin` - load firmware
> `monitor mdw 0x10000000 1` - read first word from RAM  
> `monitor bp 0x00000088 2 hw` - break at reset ahnder (see address for `Reset_Handler` in map file)  
> `monitor rbp 0x00000088` - remove breakpoint  
> `monitor bp` - list breakpoints
> `monitor step` - step a single instrauction
> `monitor step 3` - step 3 instructions
> `next count [n]` - step next line of code (regardless of number of instructions) [n] is the number of lines to step
> `finish` - continue running until just after function in the selected stack frame returns
> `info registers` - list registers
> `list` - list file
> `x/20x 0x0` - list vector table
> `x /wx 0xE000ED04` - read IPSR
> `arm-none-eabi-nm -S --print-size firmware/firmware.elf` - list symbols
> `arm-none-eabi-objdump -t firmware/firmware.elf` - list symbols (alternate)
> `arm-none-eabi-objdump -s -j .isr_vector firmware/firmware.elf` - show isr vector
> `arm-none-eabi-objdump -D -j .isr_vector firmware/firmware.elf` - disassemble vector table


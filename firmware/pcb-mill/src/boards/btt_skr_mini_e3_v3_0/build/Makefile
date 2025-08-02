# OS-specific commands and separator
ifeq ($(OS),Windows_NT)
  DELCMD = del /Q
  MKDIR  = if not exist "$(dir $@)" mkdir "$(dir $@)"
  RM     = rmdir /S /Q
  SEP    = /
else
  DELCMD = rm -rf
  MKDIR  = mkdir -p $(dir $@)
  RM     = rm -rf
  SEP    = /
endif

# Absolute path to directory of this Makefile
MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# OBJ_DIR is under project roots so need to traverse from
# src/boards/btt_skr_mini_e3_v3_0/build to firmware
OBJ_DIR := ../../../../firmware
BLD_DIR := $(MAKEFILE_DIR)

# Output files
OUT_ELF := $(OBJ_DIR)$(SEP)pcb-mill.elf
OUT_MAP := $(OBJ_DIR)$(SEP)pcb-mill.map
OUT_BIN := $(OBJ_DIR)$(SEP)pcb-mill.bin

# Flash binary with OpenOCD
.PHONY: flash
flash:
	@echo "[FLASH] Flashing binary"
	openocd -f interface/stlink.cfg -f target/stm32g0x.cfg \
		-c "adapter speed 500; program $(OUT_BIN) verify reset exit 0x08000000"

# Run OpenOCD with flashing and halt for debugging
.PHONY: debug
debug:
	@echo "[OPENOCD] Programming and halting"
	openocd -f interface/stlink.cfg -f target/stm32g0x.cfg \
		-c "adapter speed 500;" \
		-c "program $(OUT_BIN) verify" \
		-c "reset halt"

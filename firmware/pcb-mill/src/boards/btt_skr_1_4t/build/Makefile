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
BLD_DIR := $(MAKEFILE_DIR)

# Put build outputs under firmware/<BOARD_NAME>/
OBJ_DIR_BASE  := ../../../../firmware
OBJ_DIR       := $(OBJ_DIR_BASE)$(SEP)$(BOARD_NAME)

# Output files
OUT_ELF := $(OBJ_DIR)$(SEP)pcb-mill.elf
OUT_MAP := $(OBJ_DIR)$(SEP)pcb-mill.map
OUT_BIN := $(OBJ_DIR)$(SEP)pcb-mill.bin

.PHONY: checksum
checksum:
	@echo "[CHK] Fixing vector checksum"
	python "$(BLD_DIR)$(SEP)checksum.py" "$(OUT_BIN)"
	python "$(BLD_DIR)$(SEP)verify_checksum.py" "$(OUT_BIN)"

# Flash binary with OpenOCD
.PHONY: flash
flash: checksum
	@echo "[FLASH] Flashing binary"
	openocd -f interface/stlink.cfg -f target/lpc17xx.cfg \
		-c "debug_level 1; adapter speed 1800; program $(abspath $(OUT_BIN)) verify reset exit 0x0"

# Run OpenOCD: program ELF, then halt and wait for GDB
.PHONY: debug
debug:
	@echo [OPENOCD] Programming and halting
	"openocd" -f interface/stlink.cfg -f target/lpc17xx.cfg \
		-c "debug_level 1" \
		-c "adapter speed 1800" \
		-c "init" \
		-c "halt" \
		-c "program {$(abspath $(OUT_ELF))} verify" \
		-c "reset halt"


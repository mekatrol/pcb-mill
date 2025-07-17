# Source code structure
# src/
# ├── boards/
# │   └── btt_e3_min_v3_0/
# │       ├── startup.s
# │       └── board.c
# ├── drivers/
# │   ├── uart/
# │   │   └── uart.c
# │   ├── timers/
# │   │   └── timer.c
# │   └── tmc2209/
# │       └── tmc2209.c

# Project configuration
BOARD_NAME := btt_e3_min_v3_0
DRIVER_NAMES := fans logging timers tmc2209 uart

# OS-specific commands and separator
ifeq ($(OS),Windows_NT)
  DELCMD = del /Q
  MKDIR  = if not exist "$(dir $@)" mkdir "$(dir $@)"
  RM     = rmdir /S /Q
  SEP    = \\
else
  DELCMD = rm -rf
  MKDIR  = mkdir -p $(dir $@)
  RM     = rm -rf
  SEP    = /
endif

# CPU 
MCU = cortex-m0plus
CHIP = STM32G0B1

# Toolchain
CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-as
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size
NM      := arm-none-eabi-nm
OBJDUMP := arm-none-eabi-objdump
READELF := arm-none-eabi-readelf

# Directories
SRC_DIR := src
OBJ_DIR := firmware
BLD_DIR := build

# Output files
OUT_ELF := $(OBJ_DIR)$(SEP)firmware.elf
OUT_MAP := $(OBJ_DIR)$(SEP)firmware.map
OUT_BIN := $(OBJ_DIR)$(SEP)firmware.bin

# Linker script
LD_SCRIPT := $(BLD_DIR)$(SEP)linker.ld

# Compiler flags
CFLAGS := -mcpu=$(MCU) -mthumb -g -O0 -ffreestanding -nostdlib -Wall # -Wno-unused-variable

# Assembler flags
ASFLAGS := -mcpu=$(MCU) -mthumb -g

# Linker flags
LDFLAGS := -mcpu=$(MCU) -mthumb -nostartfiles -Wl,-Map=$(OUT_MAP) -T $(LD_SCRIPT)

# Include paths
BOARD_DIR := $(SRC_DIR)/boards/$(BOARD_NAME)
DRIVER_DIRS := $(foreach name,$(DRIVER_NAMES),$(SRC_DIR)/drivers/$(name))

CFLAGS += -I$(BOARD_DIR)
CFLAGS += $(foreach dir,$(DRIVER_DIRS),-I$(dir))

# Source files
CSRCS := $(wildcard $(SRC_DIR)/grbl/*.c) \
         $(wildcard $(BOARD_DIR)/*.c) \
         $(foreach dir,$(DRIVER_DIRS),$(wildcard $(dir)/*.c))

ASRCS := $(wildcard $(SRC_DIR)/grbl/*.s) \
         $(wildcard $(BOARD_DIR)/*.s) \
         $(foreach dir,$(DRIVER_DIRS),$(wildcard $(dir)/*.s))

SRCS := $(CSRCS) $(ASRCS)

# Object files
C_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CSRCS))
A_OBJS := $(patsubst $(SRC_DIR)/%.s,$(OBJ_DIR)/%.o,$(ASRCS))
OBJS   := $(A_OBJS) $(C_OBJS)

# Default target
.PHONY: all
all: $(OUT_BIN)

# Create object directory if needed
$(OBJ_DIR):
	@$(MKDIR)

# Compile C sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "[CC] $<"
	@$(MKDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Assemble ASM sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s | $(OBJ_DIR)
	@echo "[AS] $<"
	@$(MKDIR)
	$(AS) $(ASFLAGS) -o $@ $<

# Link ELF
$(OUT_ELF): $(OBJS) | $(OBJ_DIR)
	@echo "[LD] Linking $@"
	$(CC) $(LDFLAGS) -o $@ $^

# Generate binary
$(OUT_BIN): $(OUT_ELF)
	@echo "[OBJCOPY] Generating binary $@"
	$(OBJCOPY) -O binary $< $@

# Flash binary with OpenOCD
.PHONY: flash
flash:
	@echo "[FLASH] Flashing binary"
	openocd -f interface/stlink.cfg -f target/stm32g0x.cfg \
		-c "adapter speed 500; program $(OUT_BIN) verify reset exit 0x08000000"

# Run OpenOCD with flashing and halt for debugging
.PHONY: openocd
openocd:
	@echo "[OPENOCD] Programming and halting"
	openocd -f interface/stlink.cfg -f target/stm32g0x.cfg \
		-c "adapter speed 500;" \
		-c "program $(OUT_BIN) verify" \
		-c "reset halt"

# Run OpenOCD in background for GDB debugging
.PHONY: openocd-debug
openocd-debug:
	@start "" openocd -f interface/stlink.cfg -f target/stm32g0x.cfg -c "adapter speed 500"

# Debug target (runs OpenOCD and gdb)
.PHONY: debug
debug: $(OUT_ELF)
	@echo "[DEBUG] Starting OpenOCD and GDB"
	arm-none-eabi-gdb -x $(BLD_DIR)$(SEP)debug.gdb $(OUT_ELF)

# Utility targets
.PHONY: filesize sections dump symbols
filesize:
	$(SIZE) $(OUT_ELF)

sections:
	$(READELF) -l $(OUT_ELF)

dump:
	$(OBJDUMP) -h $(OUT_ELF)

symbols:
	$(NM) $(OUT_ELF)

# Clean build artifacts
.PHONY: clean
clean:
	@echo "[CLEAN] Removing build artifacts"
	-$(RM) "$(OBJ_DIR)"

# Reset flash
.PHONY: reset
reset:
	openocd -f interface/stlink.cfg -f target/stm32g0x.cfg -c "init; reset halt; flash write_image erase firmware/firmware.bin 0x08000000; reset run; exit"

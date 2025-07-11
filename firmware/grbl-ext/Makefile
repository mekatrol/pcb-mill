# OS-specific commands and separator
ifeq ($(OS),Windows_NT)
  DELCMD = del /Q
  MKDIR  = if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"
  RM     = rmdir /S /Q
  SEP    = \\
else
  DELCMD = rm -rf
  MKDIR  = mkdir -p "$(OBJ_DIR)"
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
CFLAGS := -mcpu=$(MCU) -mthumb -g -O0 -ffreestanding -nostdlib -Wall

# Assembler flags
ASFLAGS := -mcpu=$(MCU) -mthumb -g 

# Linker flags
LDFLAGS := -mcpu=$(MCU) -mthumb -nostartfiles -Wl,-Map=$(OUT_MAP) -T $(LD_SCRIPT)

# Sources
CSRCS := $(wildcard $(SRC_DIR)/*.c)
ASRCS := $(wildcard $(SRC_DIR)/*.s)
SRCS  := $(CSRCS) $(ASRCS)

# Objects
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
	$(CC) $(CFLAGS) -c -o $@ $<

# Assemble ASM sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s | $(OBJ_DIR)
	@echo "[AS] $<"
	$(AS) $(ASFLAGS) -o $@ $<

# Link ELF
$(OUT_ELF): $(OBJS)
	@echo "[LD] Linking $@"
	$(CC) $(LDFLAGS) -o $@ $^

# Generate binary
$(OUT_BIN): $(OUT_ELF)
	@echo "[OBJCOPY] Generating binary $@"
	$(OBJCOPY) -O binary $< $@

# Flash binary with OpenOCD
.PHONY: flash
flash: # $(OUT_BIN)
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

# Run OpenOCD with flashing and halt for debugging
.PHONY: openocd-debug
openocd-debug: 
	@start "" openocd -f interface/stlink.cfg -f target/stm32g0x.cfg -c "adapter speed 500"

# Debug target (runs openocd and gdb)
.PHONY: debug
debug: $(OUT_ELF)
	@echo "[DEBUG] Starting OpenOCD and GDB"
	arm-none-eabi-gdb -x $(BLD_DIR)$(SEP)debug.gdb $(OUT_ELF)

# Utility commands for info
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

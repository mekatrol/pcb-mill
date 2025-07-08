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
LD_SCRIPT := $(BLD_DIR)$(SEP)lpc1769.ld

# Compiler flags
CFLAGS := -mcpu=cortex-m3 -mthumb -g -O0 -ffreestanding -nostdlib -Wall -Wno-unused-variable

# Assembler flags
ASFLAGS := -mcpu=cortex-m3 -mthumb -g

# Linker flags
LDFLAGS := -mcpu=cortex-m3 -mthumb -nostartfiles -Wl,-Map=$(OUT_MAP) -T $(LD_SCRIPT)

# Sources
CSRCS := $(wildcard $(SRC_DIR)/*.c)
ASRCS := $(wildcard $(SRC_DIR)/*.s)
SRCS  := $(CSRCS) $(ASRCS)

# Objects
C_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CSRCS))
A_OBJS := $(patsubst $(SRC_DIR)/%.s,$(OBJ_DIR)/%.o,$(ASRCS))
OBJS   := $(C_OBJS) $(A_OBJS)

# Default target
.PHONY: all
all: $(OUT_BIN) checksum

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

# Checksum depends on bin
.PHONY: checksum
checksum: $(OUT_BIN)
	@echo "[CHK] Fixing vector checksum"
	python "$(BLD_DIR)$(SEP)checksum.py" "$(OUT_BIN)"
	python "$(BLD_DIR)$(SEP)verify_checksum.py" "$(OUT_BIN)"

# Flash binary with OpenOCD
.PHONY: flash
flash: # $(OUT_BIN)
	@echo "[FLASH] Flashing binary"
	openocd -f interface/stlink.cfg -f target/lpc17xx.cfg \
		-c "adapter speed 500; program $(OUT_BIN) verify reset exit 0x0"

# Run OpenOCD with flashing and halt for debugging
.PHONY: openocd
openocd: 
	@echo "[OPENOCD] Programming and halting"
	openocd -f interface/stlink.cfg -f target/lpc17xx.cfg \
		-c "adapter speed 500" \
		-c "program $(OUT_BIN) verify" \
		-c "reset halt"

# Debug target (runs openocd and gdb)
.PHONY: debug
debug: $(OUT_ELF)
	@echo "[DEBUG] Starting OpenOCD and GDB"
	start cmd /c "openocd -f interface/stlink.cfg -f target/lpc17xx.cfg -c 'adapter speed 500' -c 'program $(OUT_ELF) verify' -c 'reset halt'"
	timeout /t 2 >nul
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

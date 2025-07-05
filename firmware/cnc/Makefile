# OS-specific delete
ifeq ($(OS),Windows_NT)
  DELCMD = del /Q
  SEP     = \\
else
  DELCMD = rm -rf
  SEP     = /
endif

# Toolchain
CC       := arm-none-eabi-gcc
OBJCOPY  := arm-none-eabi-objcopy

# Directories
SRC_DIR  := src
OBJ_DIR  := firmware
BLD_DIR  := build

# Output names
OUT_ELF  := $(OBJ_DIR)$(SEP)firmware.elf
OUT_MAP  := $(OBJ_DIR)$(SEP)firmware.map
OUT_BIN  := $(OBJ_DIR)$(SEP)firmware.bin

# Linker script
LD_SCRIPT := build/lpc1769.ld

# Flags
CFLAGS   := -c \
             -mcpu=cortex-m3 \
             -mthumb \
             -O2 \
             -ffreestanding \
             -nostdlib \
             -Wall

LDFLAGS  := -mcpu=cortex-m3 \
             -mthumb \
             -nostartfiles \
             -Wl,-Map=$(OUT_MAP) \
             -T $(LD_SCRIPT)

# Gather sources & objects
SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Default: compile & link
all: $(OUT_BIN) checksum

# Ensure obj directory exists
$(OBJ_DIR):
ifeq ($(OS),Windows_NT)
	@if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"
else
	@mkdir -p "$(OBJ_DIR)"
endif

# Compile only (no link)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo [1] Compiling source files...
	$(CC) $(CFLAGS) -o $@ $<

# Link (and emit .map)
$(OUT_ELF): $(OBJS)
	@echo [2] Linking...
	$(CC) $(LDFLAGS) -o $@ $^

# Generate binary from ELF
bin: $(OUT_BIN)

$(OUT_BIN): $(OUT_ELF)
	@echo [3] Generating binary...
	$(OBJCOPY) -O binary $< $@

# Run checksum script on binary
checksum: $(OUT_BIN)
	@echo [4] Fixing LPC1769 vector checksum...	
	python "$(BLD_DIR)$(SEP)checksum.py" "$(OUT_BIN)"

# Flash firmware to target device
flash: $(OUT_BIN)
	@echo Flashing firmware...
	openocd -f interface/stlink.cfg -f target/lpc17xx.cfg \
	  -c "adapter speed 500; program $(OUT_BIN) verify reset exit 0x0"

filesize:
	arm-none-eabi-size $(OUT_ELF)

sections:
	arm-none-eabi-readelf -l $(OUT_ELF)

# Clean
clean:
ifeq ($(OS),Windows_NT)
	@echo Cleaning Windows build artifacts...
	@if exist "$(OBJ_DIR)" rmdir /S /Q "$(OBJ_DIR)"
else
	@echo Cleaning Unix build artifacts...
	@rm -rf "$(OBJ_DIR)"
endif


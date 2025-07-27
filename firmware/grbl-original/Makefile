# Project configuration
MCU = cortex-m0plus
CHIP = STM32G0B1
BOARD_NAME := btt_skr_mini_e3_v3_0
DRIVER_NAMES := 

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

# Toolchain
CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-as
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size
NM      := arm-none-eabi-nm
OBJDUMP := arm-none-eabi-objdump
READELF := arm-none-eabi-readelf

# Directories
LIB_DIR := libs
SRC_DIR := src
OBJ_DIR := firmware
BLD_DIR := build
BOARD_DIR := $(SRC_DIR)/boards/$(BOARD_NAME)
BOARD_BLD_DIR := $(BOARD_DIR)/build
DRIVER_DIRS := $(foreach name,$(DRIVER_NAMES),$(SRC_DIR)/drivers/$(name))
TINYUSB_DIR := $(LIB_DIR)/tinyusb

# Output files
OUT_ELF := $(OBJ_DIR)$(SEP)grbl.elf
OUT_MAP := $(OBJ_DIR)$(SEP)grbl.map
OUT_BIN := $(OBJ_DIR)$(SEP)grbl.bin

# Compiler flags
CFLAGS := -mcpu=$(MCU) -mthumb -g -O0 -ffreestanding -nostdlib -Wall -Werror # -Wno-unused-variable

# Assembler flags
ASFLAGS := -mcpu=$(MCU) -mthumb -g

# Linker script
LD_SCRIPT := $(BOARD_DIR)$(SEP)build$(SEP)linker.ld

# Linker flags
LDFLAGS := -mcpu=$(MCU) -mthumb -nostartfiles -Wl,-Map=$(OUT_MAP),--fatal-warnings,-T$(LD_SCRIPT)

CFLAGS += -I$(SRC_DIR)/grbl
CFLAGS += -I$(SRC_DIR)/grbl/include
CFLAGS += -I$(BOARD_DIR) -I$(BOARD_DIR)/include -I$(BOARD_DIR)/include/core
CFLAGS += -I$(TINYUSB_DIR)/src -I$(TINYUSB_DIR)/src/common -I$(TINYUSB_DIR)/src/class/cdc 
CFLAGS += -I$(TINYUSB_DIR)/src/device -I$(TINYUSB_DIR)/src/portable -I$(TINYUSB_DIR)/src/portable/st/stm32_fsdev

# Core sources
CSRCS := $(wildcard $(SRC_DIR)/grbl/*.c) \
         $(wildcard $(BOARD_DIR)/*.c)

ASRCS := $(wildcard $(SRC_DIR)/grbl/*.s) \
         $(wildcard $(BOARD_DIR)/*.s)

# Add driver paths and includes if any drivers defined
ifneq ($(strip $(DRIVER_NAMES)),)
  CFLAGS += $(foreach dir,$(DRIVER_DIRS),-I$(dir) -I$(dir)/include)
  CSRCS  += $(foreach dir,$(DRIVER_DIRS),$(wildcard $(dir)/*.c))
  ASRCS  += $(foreach dir,$(DRIVER_DIRS),$(wildcard $(dir)/*.s))
endif

CSRCS += \
  $(TINYUSB_DIR)/src/tusb.c \
  $(TINYUSB_DIR)/src/common/tusb_fifo.c \
  $(TINYUSB_DIR)/src/device/usbd.c \
  $(TINYUSB_DIR)/src/device/usbd_control.c \
  $(TINYUSB_DIR)/src/class/cdc/cdc_device.c 

# Add MCU CHIP specific files
ifeq ($(CHIP),STM32G0B1)
  CFLAGS += -DCFG_TUSB_MCU=OPT_MCU_STM32G0
  CSRCS += $(TINYUSB_DIR)/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
endif

SRCS := $(CSRCS) $(ASRCS)

# Object files
C_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(CSRCS))
A_OBJS := $(patsubst %.s,$(OBJ_DIR)/%.o,$(subst $(SRC_DIR)/,,$(ASRCS)))
OBJS   := $(A_OBJS) $(C_OBJS)

# Default target
.PHONY: all
all: $(OUT_BIN)

# Create object directory if needed
$(OBJ_DIR):
	@$(MKDIR)

# Compile C sources
$(OBJ_DIR)/%.o: %.c
	@echo "[CC] $<"
	@$(MKDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Assemble ASM sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s | $(OBJ_DIR)
	@echo "[AS] $<"
	@$(MKDIR)
	$(AS) $(ASFLAGS) -o $@ $<

$(OBJ_DIR)/lib/%.o: lib/%.c | $(OBJ_DIR)
	@echo "[CC] $<"
	@$(MKDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

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
	$(MAKE) -C $(BOARD_BLD_DIR) flash

# Debug binary with OpenOCD
.PHONY: flash
debug:
	@echo "[DEBUG] Launching OpenOCD/gdb"
	$(MAKE) -C $(BOARD_BLD_DIR) debug

# Clean build artifacts
.PHONY: clean
clean:
	@echo "[CLEAN] Removing build artifacts"
	-$(RM) "$(OBJ_DIR)"

.PHONY: build
build:
	$(MAKE) clean
	$(MAKE) $(OUT_BIN)

.PHONY: vscode_cpp_settings
vscode_cpp_settings:
	@echo "[TOOLS] Generate CPP setting"
	python tools/gen_c_cpp_properties.py


# =========================
# Project configuration (defaults; CLI can override)
# =========================
MCU        ?= cortex-m0plus
CHIP       ?= STM32G0B1
BOARD_NAME ?= btt_skr_mini_e3_v3_0
DRIVER_NAMES ?= usb usb_cdc

# Export to sub-makes
export MCU CHIP BOARD_NAME DRIVER_NAMES

# =========================
# OS-specific shell/commands
# =========================
ifeq ($(OS),Windows_NT)
  SHELL := cmd
  .SHELLFLAGS := /c
  DELCMD = del /Q
  # Ensure mkdir works with forward slashes by converting to backslashes
  MKDIR  = if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
  RM     = rmdir /S /Q
  SEP    = /
else
  DELCMD = rm -f
  MKDIR  = mkdir -p $(dir $@)
  RM     = rm -rf
  SEP    = /
endif

# =========================
# Toolchain
# =========================
CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-as
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size
NM      := arm-none-eabi-nm
OBJDUMP := arm-none-eabi-objdump
READELF := arm-none-eabi-readelf

# =========================
# Directories
# =========================
LIB_DIR       := libs
SRC_DIR       := src
BLD_DIR       := build
BOARD_DIR     := $(SRC_DIR)/boards/$(BOARD_NAME)
BOARD_BLD_DIR := $(BOARD_DIR)/build
DRIVER_DIRS   := $(foreach name,$(DRIVER_NAMES),$(SRC_DIR)/drivers/$(name))

# Put build outputs under firmware/<BOARD_NAME>/
OBJ_DIR_BASE  := firmware
OBJ_DIR       := $(OBJ_DIR_BASE)$(SEP)$(BOARD_NAME)

# =========================
# Outputs
# =========================
OUT_ELF := $(OBJ_DIR)$(SEP)pcb-mill.elf
OUT_MAP := $(OBJ_DIR)$(SEP)pcb-mill.map
OUT_BIN := $(OBJ_DIR)$(SEP)pcb-mill.bin

# =========================
# Flags
# =========================
CFLAGS  := -mcpu=$(MCU) -mthumb -g -O0 -ffreestanding -nostdlib -Wall -Werror -Wno-error=switch -Wno-switch
CFLAGS  += -ffunction-sections -fdata-sections
ASFLAGS := -mcpu=$(MCU) -mthumb -g
LD_SCRIPT := $(BOARD_DIR)$(SEP)build$(SEP)linker.ld
LDFLAGS := -mcpu=$(MCU) -mthumb -nostartfiles -Wl,-Map=$(OUT_MAP),--fatal-warnings,-T$(LD_SCRIPT)
LDFLAGS += -Wl,--gc-sections

# (Optional) If you build Cortex-M4F like STM32F407 with HW FPU, add:
# CFLAGS  += -mfpu=fpv4-sp-d16 -mfloat-abi=hard
# LDFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Includes
CFLAGS += -I$(SRC_DIR)/config -I$(SRC_DIR)/core -I$(SRC_DIR)/core/include
CFLAGS += -I$(BOARD_DIR) -I$(BOARD_DIR)/include -I$(BOARD_DIR)/include/core

# =========================
# Sources
# =========================
CSRCS := $(wildcard $(SRC_DIR)/core/*.c) \
         $(wildcard $(BOARD_DIR)/*.c)
ASRCS := $(wildcard $(SRC_DIR)/core/*.s) \
         $(wildcard $(BOARD_DIR)/*.s)

# Drivers
ifneq ($(strip $(DRIVER_NAMES)),)
  CFLAGS += $(foreach dir,$(DRIVER_DIRS),-I$(dir) -I$(dir)/include)
  CSRCS  += $(foreach dir,$(DRIVER_DIRS),$(wildcard $(dir)/*.c))
  ASRCS  += $(foreach dir,$(DRIVER_DIRS),$(wildcard $(dir)/*.s))
endif

SRCS := $(CSRCS) $(ASRCS)

# =========================
# Objects
# =========================
C_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(CSRCS))
A_OBJS := $(patsubst %.s,$(OBJ_DIR)/%.o,$(ASRCS))
OBJS   := $(A_OBJS) $(C_OBJS)

# =========================
# Default target
# =========================
.PHONY: all
all: $(OUT_BIN)

# Ensure object directory exists (top-level)
$(OBJ_DIR):
	@$(MKDIR)

# Compile C
$(OBJ_DIR)/%.o: %.c
	@echo [CC] $<
	@$(MKDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Assemble ASM
$(OBJ_DIR)/%.o: %.s | $(OBJ_DIR)
	@echo [AS] $<
	@$(MKDIR)
	$(AS) $(ASFLAGS) -o $@ $<

# Link ELF
$(OUT_ELF): $(OBJS) | $(OBJ_DIR)
	@echo [LD] Linking $@
	$(CC) $(LDFLAGS) -o $@ $^

# Generate BIN
$(OUT_BIN): $(OUT_ELF)
	@echo [OBJCOPY] Generating $@
	$(OBJCOPY) -O binary $< $@

# =========================
# Tools
# =========================
.PHONY: size
size: $(OUT_ELF)
	$(SIZE) $<

# =========================
# Flash / Debug (delegated to board build dir)
# =========================
.PHONY: flash
flash:
	@echo [FLASH] $(CHIP) on $(BOARD_NAME)
	$(MAKE) -C $(BOARD_BLD_DIR) flash MCU=$(MCU) CHIP=$(CHIP) BOARD_NAME=$(BOARD_NAME) DRIVER_NAMES="$(DRIVER_NAMES)"

.PHONY: debug
debug:
	@echo [DEBUG] $(CHIP) on $(BOARD_NAME)
	$(MAKE) -C $(BOARD_BLD_DIR) debug MCU=$(MCU) CHIP=$(CHIP) BOARD_NAME=$(BOARD_NAME) DRIVER_NAMES="$(DRIVER_NAMES)"

# =========================
# Clean / Build
# =========================
.PHONY: clean
clean:
	@echo [CLEAN] Removing build artifacts
	-$(RM) "$(OBJ_DIR)"

# Nuke all boards’ builds (not just the current BOARD_NAME)
.PHONY: cleanall
cleanall:
	@echo [CLEAN] Removing ALL build artifacts
	-$(RM) "$(OBJ_DIR_BASE)"

.PHONY: build
build:
	$(MAKE) clean
	$(MAKE) $(OUT_BIN)

.PHONY: vscode_cpp_settings
vscode_cpp_settings:
	@echo "[TOOLS] Generate CPP setting"
	python tools/gen_c_cpp_properties.py


# =========================
# Config echo
# =========================
.PHONY: print-config
print-config:
	@echo MCU=$(MCU)
	@echo CHIP=$(CHIP)
	@echo BOARD_NAME=$(BOARD_NAME)
	@echo DRIVER_NAMES=$(DRIVER_NAMES)
	@echo BOARD_DIR=$(BOARD_DIR)

# =========================
# Board-specific meta targets (no double-clean)
# =========================
.PHONY: btt_skr_1_4t
btt_skr_1_4t:
	$(MAKE) build MCU=cortex-m3 CHIP=LPC1769 BOARD_NAME=btt_skr_1_4t

.PHONY: btt_skr_mini_e3_v3_0
btt_skr_mini_e3_v3_0:
	$(MAKE) build MCU=cortex-m0plus CHIP=STM32G0B1 BOARD_NAME=btt_skr_mini_e3_v3_0

.PHONY: stm32f407g
stm32f407g:
	$(MAKE) build MCU=cortex-m4 CHIP=STM32F407 BOARD_NAME=stm32f407g_disc1

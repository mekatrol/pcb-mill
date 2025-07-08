TARGET = led_rotate
BUILD_DIR = firmware
SRC_DIR = src
OUT_MAP  := $(TARGET).map

AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

CFLAGS = -g -O0 -Wall -mcpu=cortex-m4 -mthumb -ffreestanding -I$(SRC_DIR)
LDFLAGS = -T linker.ld -nostdlib -g -O0 -nostartfiles -nodefaultlibs -nostdlib -Wl,-Map=$(BUILD_DIR)/$(OUT_MAP)

OBJS = $(BUILD_DIR)/startup.o $(BUILD_DIR)/main.o

all: $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/startup.o: $(SRC_DIR)/startup.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@ 


$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	$(SIZE) $<

clean:
	del /Q $(BUILD_DIR)\*.o $(BUILD_DIR)\*.elf $(BUILD_DIR)\*.bin 2>nul || exit 0

flash:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program $(BUILD_DIR)/$(TARGET).bin 0x08000000 verify reset exit"

debug:
	cmd /c start "" openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
	timeout /t 2 >nul
	arm-none-eabi-gdb $(BUILD_DIR)/$(TARGET).elf 

# target extended-remote localhost:3333			# Connect to device
# monitor reset init       						# Reset and re-init the MCU
# monitor halt             						# Ensure it is halted
# load                     						# Optional: reload the ELF (if not already flashed)
# break main               						# Set breakpoint at main
# continue                 						# Resume execution

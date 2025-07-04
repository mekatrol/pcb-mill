cmake_minimum_required(VERSION 3.13)
project(grblHAL_LPC176x C CXX ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# Output directory
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/../output")
file(MAKE_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# Toolchain setup (should normally be in toolchain file)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m3)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Compiler
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(OBJCOPY arm-none-eabi-objcopy)

# Flags
set(COMMON_FLAGS "-mcpu=cortex-m3 -mthumb -Os -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS "${COMMON_FLAGS} -Wall -Wextra -std=gnu99")
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -Wall -Wextra -std=gnu++11")

# Defines
add_compile_definitions(
    __LPC176x__
    __LPC17XX__
    CORE_M3
    USB_SERIAL_CDC
    GRBLHAL
    HAL_LPC176X
    BOARD_BTT_SKR_TURBO
    STATIC_ALLOCATION
    NO_BOARD_LIB
    DEFAULT_CPU_CLOCK=120000000
)

# Includes
include_directories(
    ../LPC176x/src
    ../LPC176x/src/lpc_chip_175x_6x/inc
    ../LPC176x/src/lpc_chip_175x_6x/inc/usbd
    ../LPC176x/src/FatFs
)

# Source collection
file(GLOB_RECURSE C_SOURCES "../LPC176x/src/*.c")
file(GLOB_RECURSE CPP_SOURCES "../LPC176x/src/*.cpp")

add_executable(firmware.elf ${C_SOURCES} ${CPP_SOURCES})

# Linker script
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/lpc1769.ld")
target_link_options(firmware.elf PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -lm
)

# Custom command to generate .bin file
add_custom_command(TARGET firmware.elf POST_BUILD
    COMMAND ${OBJCOPY} -O binary ${OUTPUT_DIR}/firmware.elf ${OUTPUT_DIR}/firmware.bin
    COMMENT "Generating firmware.bin"
)

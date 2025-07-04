# arm-none-eabi-toolchain.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m3)

# Define the cross compiler
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# Set default flags
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m3 -mthumb")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m3 -mthumb")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-mcpu=cortex-m3 -mthumb")

# Prevent CMake from trying to link test programs
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

@echo off
setlocal

REM === Tools ===
set CC=arm-none-eabi-gcc
set OBJCOPY=arm-none-eabi-objcopy

REM === Output Directory ===
set OUTDIR=firmware
set OUT=%OUTDIR%\firmware
set SRC=src

REM Clean output folder before build
if exist %OUTDIR% (
    echo Cleaning firmware folder %BUILD%...
    rmdir /s /q %OUTDIR%
)
mkdir %OUTDIR%

if not exist %OUTDIR% (
    mkdir %OUTDIR%
)

echo [1] Compiling sources...
%CC% -c %SRC%\main.c -o %OUTDIR%\main.o -mcpu=cortex-m3 -mthumb -O2 -ffreestanding -nostdlib -Wall -Wextra
%CC% -c %SRC%\syscalls.c -o %OUTDIR%\syscalls.o -mcpu=cortex-m3 -mthumb -O2 -ffreestanding -nostdlib -Wall -Wextra
%CC% -c %SRC%\systick.c -o %OUTDIR%\systick.o -mcpu=cortex-m3 -mthumb -O2 -ffreestanding -nostdlib -Wall -Wextra
%CC% -c %SRC%\startup.c -o %OUTDIR%\startup.o -mcpu=cortex-m3 -mthumb -O2 -ffreestanding -nostdlib -Wall -Wextra
%CC% -c %SRC%\uart.c -o %OUTDIR%\uart.o -mcpu=cortex-m3 -mthumb -O2 -ffreestanding -nostdlib -Wall -Wextra


echo [2] Linking...
%CC% -T "%~dp0lpc1769.ld" -o %OUT%.elf ^
  %OUTDIR%\startup.o ^
  %OUTDIR%\main.o ^
  %OUTDIR%\systick.o ^
  %OUTDIR%\syscalls.o ^
  %OUTDIR%\uart.o ^
  -mcpu=cortex-m3 -mthumb ^
  -Wl,-Map=%OUT%.map -nostartfiles


echo [3] Generating binary...
%OBJCOPY% -O binary %OUT%.elf %OUT%.bin

echo [4] Fixing LPC1769 vector checksum...
python "%~dp0checksum.py" "%~dp0..\%OUT%.bin"

echo [5] Checking size...
arm-none-eabi-size %OUT%.elf

echo [5] Checking sections...
arm-none-eabi-readelf -l %OUT%.elf

echo Done.

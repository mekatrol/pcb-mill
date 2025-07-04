openocd -f interface/stlink.cfg -f target/lpc17xx.cfg -c "adapter speed 500; program firmware/firmware.bin verify reset exit 0x0"

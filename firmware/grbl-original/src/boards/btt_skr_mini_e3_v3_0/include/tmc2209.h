#ifndef __TMC2209_H__
#define __TMC2209_H__

#include <stdint.h>

// TMC2209 Register Addresses
#define TMC2209_REG_GCONF 0x00       // General configuration
#define TMC2209_REG_GSTAT 0x01       // Global status
#define TMC2209_REG_IFCNT 0x03       // Interface transmission counter
#define TMC2209_REG_SLAVECONF 0x04   // Slave configuration
#define TMC2209_REG_IOIN 0x06        // Input/output status
#define TMC2209_REG_IHOLD_IRUN 0x10  // Current control (hold/run/boost)
#define TMC2209_REG_TPOWERDOWN 0x11  // Power-down delay
#define TMC2209_REG_TPWMTHRS 0x13    // StealthChop activation threshold
#define TMC2209_REG_TCOOLTHRS 0x14   // CoolStep activation threshold
#define TMC2209_REG_THIGH 0x15       // Upper velocity threshold for CoolStep

// Direct coil control
#define TMC2209_REG_XDIRECT 0x20
#define TMC2209_REG_VDCMIN 0x21

// Microstep LUT
#define TMC2209_REG_MSLUT0 0x33
#define TMC2209_REG_MSLUT1 0x34
#define TMC2209_REG_MSLUT2 0x35
#define TMC2209_REG_MSLUT3 0x36
#define TMC2209_REG_MSLUT4 0x37
#define TMC2209_REG_MSLUT5 0x38
#define TMC2209_REG_MSLUT6 0x39
#define TMC2209_REG_MSLUT7 0x3A
#define TMC2209_REG_MSLUTSEL 0x3B
#define TMC2209_REG_MSLUTSTART 0x3C

// Chopper and CoolStep configuration
#define TMC2209_REG_CHOPCONF 0x6C
#define TMC2209_REG_COOLCONF 0x6D
#define TMC2209_REG_DCCTRL 0x6E
#define TMC2209_REG_DRVSTATUS 0x6F

// StealthChop and diagnostics
#define TMC2209_REG_PWMCONF 0x7A
#define TMC2209_REG_PWM_SCALE 0x7B  // Read-only
#define TMC2209_REG_ENCM_CTRL 0x7C
#define TMC2209_REG_LOST_STEPS 0x7D

void tmc2209_uart4_init();
void tmc2209_read_gconf(uint8_t slave);
int tmc2209_parse_response(uint8_t sent_count, uint8_t *data_out, uint8_t reg);
void tmc2209_tick();

#endif  // __TMC2209_H__
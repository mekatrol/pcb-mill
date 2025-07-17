#ifndef __GRBL_H__
#define __GRBL_H__

#include <stdbool.h>
#include <stdint.h>

typedef enum { ROTATION_CW, ROTATION_CCW } rotation_t;

typedef struct {
  void (*printf)(const char *fmt, ...);

  // Receive a single character (blocking)
  uint8_t (*getchar)();

  // Send a single character (blocking)
  void (*putchar)(uint8_t c);
} terminal_interface_t;

typedef struct {
  void (*delay_ms)(uint32_t ms);
} timer_interface_t;

typedef struct {
  void (*enable)();
  void (*disable)();
  void (*set_dir)(rotation_t rotation_direction);
} stepper_interface_t;

typedef struct {
  stepper_interface_t x;
  stepper_interface_t y;
  stepper_interface_t z;
} steppers_interface_t;

typedef struct {
  terminal_interface_t terminal;
  timer_interface_t timer;
  steppers_interface_t steppers;

  // The ability to enter and exit critical sections (e.g. HAL may disable and reenable interrupts)
  void (*enter_critical)();
  void (*exit_critical)();

  // The method called if there is a panic. This method should never return (it should reset the device if it can)
  void (*panic)();
} hal_interface_t;

void grbl_run(hal_interface_t *grbl);

#endif  // __GRBL_H__
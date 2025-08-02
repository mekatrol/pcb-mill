#ifndef __MOTION_H__
#define __MOTION_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int32_t cur_pos;               // Axis current position
  int32_t tgt_pos;               // Axis target position
  int32_t delta;                 // Axis position delta
  int32_t dir;                   // +1 or -1
  int32_t error;                 // for Bresenham-style interpolation
  uint8_t step_active;           // 1 if step pin is high and needs clearing
  void (*step_high)(void);       // Call HAL to set step ping high
  void (*step_low)(void);        // Call HAL to set step pin low
  void (*set_dir)(int32_t dir);  // Set stepping direction
  void (*set_ena)(bool enable);  // Set stepping enable
} Axis;

typedef struct {
  Axis x, y, z, e;
  int32_t steps_remaining;
  int32_t step_rate_ticks;  // how many timer ticks per step (inverse of speed)
  int32_t tick_counter;
  int32_t accel_ticks;
  int32_t decel_start;
  int32_t cruise_ticks;
  int32_t step_phase;
} MotionState;

extern volatile MotionState motion;

void init_motion(int32_t x, int32_t y, int32_t z, int32_t a);
void start_motion(int32_t x, int32_t y, int32_t z, int32_t a);

#endif  // __MOTION_H__
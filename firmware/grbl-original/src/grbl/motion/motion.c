#include "motion.h"

#include "../../boards/btt_skr_mini_e3_v3_0/memory_map.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))

static inline void axis_step_high(volatile Axis* a) {
  a->step_high();      // Set pin HIGH
  a->step_active = 1;  // Flag pin active
}

static inline void axis_step_low(volatile Axis* a) {
  a->step_low();       // Set pin LOW
  a->step_active = 0;  // Flag pin not active
}

void stepper_interrupt(void) {
  // Auto-clear STEP pins from previous tick
  if (motion.x.step_active) {
    axis_step_low(&motion.x);
  }
  if (motion.y.step_active) {
    axis_step_low(&motion.y);
  }
  if (motion.z.step_active) {
    axis_step_low(&motion.z);
  }

  if (motion.steps_remaining == 0)
    return;

  motion.tick_counter++;

  if (motion.tick_counter >= motion.step_rate_ticks) {
    motion.tick_counter = 0;

    // Step each axis using Bresenham logic
    if (motion.x.delta != 0) {
      motion.x.error += motion.x.delta;
      if (motion.x.error >= motion.steps_remaining) {
        motion.x.error -= motion.steps_remaining;
        axis_step_high(&motion.x);
        motion.x.current += motion.x.dir;
      }
    }

    if (motion.y.delta != 0) {
      motion.y.error += motion.y.delta;
      if (motion.y.error >= motion.steps_remaining) {
        motion.y.error -= motion.steps_remaining;
        axis_step_high(&motion.y);
        motion.y.current += motion.y.dir;
      }
    }

    if (motion.z.delta != 0) {
      motion.z.error += motion.z.delta;
      if (motion.z.error >= motion.steps_remaining) {
        motion.z.error -= motion.steps_remaining;
        axis_step_high(&motion.z);
        motion.z.current += motion.z.dir;
      }
    }

    // Advance motion
    motion.step_phase++;
    motion.steps_remaining--;

    // Acceleration / deceleration phase control
    if (motion.step_phase < motion.accel_ticks) {
      // Accel: decrease step_rate_ticks
      motion.step_rate_ticks--;
    } else if (motion.step_phase >= motion.decel_start) {
      // Decel: increase step_rate_ticks
      motion.step_rate_ticks++;
    }
  }
}

void init_motion(int32_t x, int32_t y, int32_t z) {
  motion.x.current = x;
  motion.y.current = y;
  motion.z.current = z;
}

void start_motion(int32_t x1, int32_t y1, int32_t z1) {
  // Init axes
  int32_t dx = ABS(x1 - motion.x.current);
  int32_t dy = ABS(y1 - motion.y.current);
  int32_t dz = ABS(z1 - motion.z.current);

  motion.x.target = x1;
  motion.y.target = y1;
  motion.z.target = z1;

  motion.x.delta = dx;
  motion.y.delta = dy;
  motion.z.delta = dz;

  motion.steps_remaining = dx;
  if (dy > motion.steps_remaining) motion.steps_remaining = dy;
  if (dz > motion.steps_remaining) motion.steps_remaining = dz;

  motion.x.error = motion.y.error = motion.z.error = 0;

  motion.x.dir = (x1 >= motion.x.current) ? 1 : -1;
  motion.y.dir = (y1 >= motion.y.current) ? 1 : -1;
  motion.z.dir = (z1 >= motion.z.current) ? 1 : -1;

  motion.x.set_dir(motion.x.dir);
  motion.y.set_dir(motion.y.dir);
  motion.z.set_dir(motion.z.dir);

  // Plan trapezoidal profile (basic example)
  motion.accel_ticks = motion.steps_remaining / 4;
  motion.decel_start = motion.steps_remaining - motion.accel_ticks;
  motion.step_rate_ticks = 50;  // Start slow (500 µs between steps = 2 kHz)
  motion.tick_counter = 0;
  motion.step_phase = 0;
}

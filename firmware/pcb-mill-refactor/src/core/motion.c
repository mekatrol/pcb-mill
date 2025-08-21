#include "motion.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))

static inline void axis_step_high(volatile Axis* axis) {
  axis->step_high();      // Set pin HIGH
  axis->step_active = 1;  // Flag pin active
}

static inline void axis_step_low(volatile Axis* axis) {
  axis->step_low();       // Set pin LOW
  axis->step_active = 0;  // Flag pin not active
}

void stepper_interrupt(void) {
  if (!motion.x.step_high) {
    // Motion system not yet initialised
    return;
  }

  motion.tick_counter++;

  if (motion.tick_counter % 10 != 0) {
    return;
  }

  // Auto-clear STEP pins from previous tick
  if (motion.x.step_active) {
    axis_step_low(&motion.x);
  } else {
    axis_step_high(&motion.x);
  }

  if (motion.y.step_active) {
    axis_step_low(&motion.y);
  } else {
    axis_step_high(&motion.y);
  }

  if (motion.z.step_active) {
    axis_step_low(&motion.z);
  } else {
    axis_step_high(&motion.z);
  }

  if (motion.e.step_active) {
    axis_step_low(&motion.e);
  } else {
    axis_step_high(&motion.e);
  }

  return;

  if (motion.steps_remaining == 0) {
    return;
  }

  motion.tick_counter++;

  if (motion.tick_counter >= motion.step_rate_ticks) {
    motion.tick_counter = 0;

    // Step each axis using Bresenham logic
    if (motion.x.delta != 0) {
      motion.x.error += motion.x.delta;
      if (motion.x.error >= motion.steps_remaining) {
        motion.x.error -= motion.steps_remaining;
        axis_step_high(&motion.x);
        motion.x.cur_pos += motion.x.dir;
      }
    }

    if (motion.y.delta != 0) {
      motion.y.error += motion.y.delta;
      if (motion.y.error >= motion.steps_remaining) {
        motion.y.error -= motion.steps_remaining;
        axis_step_high(&motion.y);
        motion.y.cur_pos += motion.y.dir;
      }
    }

    if (motion.z.delta != 0) {
      motion.z.error += motion.z.delta;
      if (motion.z.error >= motion.steps_remaining) {
        motion.z.error -= motion.steps_remaining;
        axis_step_high(&motion.z);
        motion.z.cur_pos += motion.z.dir;
      }
    }

    if (motion.e.delta != 0) {
      motion.e.error += motion.e.delta;
      if (motion.e.error >= motion.steps_remaining) {
        motion.e.error -= motion.steps_remaining;
        axis_step_high(&motion.e);
        motion.e.cur_pos += motion.e.dir;
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

void init_motion(int32_t x, int32_t y, int32_t z, int32_t a) {
  motion.x.cur_pos = x;
  motion.y.cur_pos = y;
  motion.z.cur_pos = z;
  motion.e.cur_pos = a;
}

void start_motion(int32_t x, int32_t y, int32_t z, int32_t a) {
  // Calcule axis deltas from their current position
  int32_t dx = ABS(x - motion.x.cur_pos);
  int32_t dy = ABS(y - motion.y.cur_pos);
  int32_t dz = ABS(z - motion.z.cur_pos);
  int32_t de = ABS(a - motion.e.cur_pos);

  motion.x.delta = dx;
  motion.y.delta = dy;
  motion.z.delta = dz;
  motion.e.delta = de;

  // Set target positions
  motion.x.tgt_pos = x;
  motion.y.tgt_pos = y;
  motion.z.tgt_pos = z;
  motion.e.tgt_pos = a;

  // Set the number of steps remaining to the largest ssteps remaining of any axis
  motion.steps_remaining = dx;
  if (dy > motion.steps_remaining) motion.steps_remaining = dy;
  if (dz > motion.steps_remaining) motion.steps_remaining = dz;
  if (de > motion.steps_remaining) motion.steps_remaining = de;

  // Start with no error
  motion.x.error = motion.y.error = motion.z.error = 0;

  // Init direction to travel based on current position relevant to target position
  motion.x.dir = (x >= motion.x.cur_pos) ? 1 : -1;
  motion.y.dir = (y >= motion.y.cur_pos) ? 1 : -1;
  motion.z.dir = (z >= motion.z.cur_pos) ? 1 : -1;
  motion.e.dir = (z >= motion.e.cur_pos) ? 1 : -1;

  motion.x.set_dir(motion.x.dir);
  motion.y.set_dir(motion.y.dir);
  motion.z.set_dir(motion.z.dir);
  motion.e.set_dir(motion.e.dir);

  // Plan trapezoidal profile
  motion.accel_ticks = motion.steps_remaining / 4;
  motion.decel_start = motion.steps_remaining - motion.accel_ticks;
  motion.step_rate_ticks = 100;  // Needs to be a minimum of 8 for bit toggle timing to work
  motion.tick_counter = 0;
  motion.step_phase = 0;
}

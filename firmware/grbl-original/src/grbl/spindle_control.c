/*
  spindle_control.c - spindle control methods
  Part of Grbl

  Copyright (c) 2012-2015 Sungeun K. Jeon
  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "grbl.h"

void spindle_init() {
  spindle_init_hal();
  spindle_stop();
}

void spindle_stop() {
  spindle_stop_hal();
}

void spindle_set_state(uint8_t state, float rpm) {
  // Halt or set spindle direction and rpm.
  if (state == SPINDLE_DISABLE || rpm <= 0.0) {
    spindle_stop();
  } else {
    spindle_set_state(state, rpm);
  }
}

void spindle_run(uint8_t state, float rpm) {
  if (sys.state == STATE_CHECK_MODE) {
    return;
  }
  protocol_buffer_synchronize();  // Empty planner buffer to ensure spindle is set when programmed.
  spindle_set_state(state, rpm);
}

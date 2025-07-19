/*
  coolant_control.c - coolant control methods
  Part of Grbl

  Copyright (c) 2012-2015 Sungeun K. Jeon

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

void coolant_init() {
  coolant_init_hal();
  coolant_stop();
}

void coolant_stop() { coolant_stop_hal(); }

void coolant_set_state(uint8_t mode) { coolant_set_state_hal(mode); }

void coolant_run(uint8_t mode) {
  if (sys.state == STATE_CHECK_MODE) {
    return;
  }
  protocol_buffer_synchronize();  // Ensure coolant turns on when specified in
                                  // program.
  coolant_set_state(mode);
}

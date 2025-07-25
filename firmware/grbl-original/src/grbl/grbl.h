/*
  grbl.h - main Grbl include file
  Part of Grbl

  Copyright (c) 2015 Sungeun K. Jeon

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

#ifndef grbl_h
#define grbl_h

// Grbl versioning system
#define GRBL_VERSION "0.9j"
#define GRBL_VERSION_BUILD "20160726"

#define F_CPU 64000000UL

// Define standard libraries used by Grbl.
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "coolant_control.h"
#include "defaults.h"
#include "eeprom.h"
#include "gcode.h"
#include "hal.h"
#include "limits.h"
#include "math_soft.h"
#include "motion/motion.h"
#include "motion_control.h"
#include "nuts_bolts.h"
#include "planner.h"
#include "print.h"
#include "probe.h"
#include "protocol.h"
#include "report.h"
#include "serial.h"
#include "settings.h"
#include "spindle_control.h"
#include "stepper.h"
#include "system.h"

#endif

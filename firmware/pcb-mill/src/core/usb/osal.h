/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#ifndef _TUSB_OSAL_H_
#define _TUSB_OSAL_H_

#include "tusb_common.h"

typedef void (*osal_task_func_t)(void*);

// Timeout
#define OSAL_TIMEOUT_NOTIMEOUT (0)              // Return immediately
#define OSAL_TIMEOUT_NORMAL (10)                // Default timeout
#define OSAL_TIMEOUT_WAIT_FOREVER (UINT32_MAX)  // Wait forever
#define OSAL_TIMEOUT_CONTROL_XFER OSAL_TIMEOUT_WAIT_FOREVER

// Mutex is required when using a preempted RTOS or MCU has multiple cores
#define OSAL_MUTEX_REQUIRED 0
#define OSAL_MUTEX_DEF(_name) \
  uint8_t:                    \
  0

// OS thin implementation
#include "osal_none.h"

#endif /* _TUSB_OSAL_H_ */

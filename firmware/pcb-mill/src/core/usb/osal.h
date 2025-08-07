#ifndef _TUSB_OSAL_H_
#define _TUSB_OSAL_H_

#include "usb.h"
#include "tusb_fifo.h"

typedef void (*osal_task_func_t)(void*);

// Timeout
typedef struct {
  void (*interrupt_set)(bool);
  tu_fifo_t ff;
} osal_queue_def_t;

typedef osal_queue_def_t* osal_queue_t;

__attribute__((always_inline)) static inline osal_queue_t osal_queue_create(osal_queue_def_t* qdef) {
  tu_fifo_clear(&qdef->ff);
  return (osal_queue_t)qdef;
}

__attribute__((always_inline)) static inline bool osal_queue_delete(osal_queue_t qhdl) {
  (void)qhdl;
  return true;  // nothing to do
}

__attribute__((always_inline)) static inline bool osal_queue_receive(osal_queue_t qhdl, void* data, uint32_t msec) {
  (void)msec;  // not used, always behave as msec = 0

  qhdl->interrupt_set(false);
  const bool success = tu_fifo_read(&qhdl->ff, data);
  qhdl->interrupt_set(true);

  return success;
}

__attribute__((always_inline)) static inline bool osal_queue_send(osal_queue_t qhdl, void const* data, bool in_isr) {
  if (!in_isr) {
    qhdl->interrupt_set(false);
  }

  const bool success = tu_fifo_write(&qhdl->ff, data);

  if (!in_isr) {
    qhdl->interrupt_set(true);
  }

  return success;
}

#endif /* _TUSB_OSAL_H_ */

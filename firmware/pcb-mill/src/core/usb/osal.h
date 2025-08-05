#ifndef _TUSB_OSAL_H_
#define _TUSB_OSAL_H_

#include "tusb_common.h"
#include "tusb_fifo.h"

typedef void (*osal_task_func_t)(void*);

// Timeout
typedef struct {
  void (*interrupt_set)(bool);
  tu_fifo_t ff;
} osal_queue_def_t;

typedef osal_queue_def_t* osal_queue_t;

#define OSAL_QUEUE_DEF(_int_set, _name, _depth, _type) \
  uint8_t _name##_buf[_depth * sizeof(_type)];         \
  osal_queue_def_t _name = {                           \
      .interrupt_set = _int_set,                       \
      .ff = TU_FIFO_INIT(_name##_buf, _depth, _type, false)}

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

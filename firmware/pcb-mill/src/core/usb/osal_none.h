#ifndef TUSB_OSAL_NONE_H_
#define TUSB_OSAL_NONE_H_

//--------------------------------------------------------------------+
// Spinlock API
//--------------------------------------------------------------------+
typedef struct {
  void (*interrupt_set)(bool);
} osal_spinlock_t;

// For SMP, spinlock must be locked by hardware, cannot just use interrupt
#define OSAL_SPINLOCK_DEF(_name, _int_set) \
  osal_spinlock_t _name = {.interrupt_set = _int_set}

__attribute__((always_inline)) static inline void osal_spin_init(osal_spinlock_t* ctx) {
  (void)ctx;
}

__attribute__((always_inline)) static inline void osal_spin_lock(osal_spinlock_t* ctx, bool in_isr) {
  if (!in_isr) {
    ctx->interrupt_set(false);
  }
}

__attribute__((always_inline)) static inline void osal_spin_unlock(osal_spinlock_t* ctx, bool in_isr) {
  if (!in_isr) {
    ctx->interrupt_set(true);
  }
}

//--------------------------------------------------------------------+
// Binary Semaphore API
//--------------------------------------------------------------------+
typedef struct {
  volatile uint16_t count;
} osal_semaphore_def_t;

typedef osal_semaphore_def_t* osal_semaphore_t;

__attribute__((always_inline)) static inline osal_semaphore_t osal_semaphore_create(osal_semaphore_def_t* semdef) {
  semdef->count = 0;
  return semdef;
}

__attribute__((always_inline)) static inline bool osal_semaphore_delete(osal_semaphore_t semd_hdl) {
  (void)semd_hdl;
  return true;  // nothing to do
}

__attribute__((always_inline)) static inline bool osal_semaphore_post(osal_semaphore_t sem_hdl, bool in_isr) {
  (void)in_isr;
  sem_hdl->count++;
  return true;
}

// TODO blocking for now
__attribute__((always_inline)) static inline bool osal_semaphore_wait(osal_semaphore_t sem_hdl, uint32_t msec) {
  (void)msec;

  while (sem_hdl->count == 0) {
  }
  sem_hdl->count--;

  return true;
}

__attribute__((always_inline)) static inline void osal_semaphore_reset(osal_semaphore_t sem_hdl) {
  sem_hdl->count = 0;
}

//--------------------------------------------------------------------+
// MUTEX API
// Within tinyusb, mutex is never used in ISR context
//--------------------------------------------------------------------+
typedef osal_semaphore_def_t osal_mutex_def_t;
typedef osal_semaphore_t osal_mutex_t;

#define osal_mutex_create(_mdef) (NULL)
#define osal_mutex_lock(_mutex_hdl, _ms) (true)
#define osal_mutex_unlock(_mutex_hdl) (true)

//--------------------------------------------------------------------+
// QUEUE API
//--------------------------------------------------------------------+
#include "tusb_fifo.h"

typedef struct {
  void (*interrupt_set)(bool);
  tu_fifo_t ff;
} osal_queue_def_t;

typedef osal_queue_def_t* osal_queue_t;

// _int_set is used as mutex in OS NONE (disable/enable USB ISR)
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

__attribute__((always_inline)) static inline bool osal_queue_empty(osal_queue_t qhdl) {
  // Skip queue lock/unlock since this function is primarily called
  // with interrupt disabled before going into low power mode
  return tu_fifo_empty(&qhdl->ff);
}

#endif

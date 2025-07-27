#include "hal.h"

#include <stdint.h>

#include "core.h"
#include "clock.h"
#include "eeprom_hal.h"
#include "gpio.h"
#include "log.h"
#include "motion.h"
#include "timers.h"
#include "tmc2209.h"
#include "tusb.h"

#include "stm32g0b1xx.h"

inline __attribute__((always_inline)) void interrupts_enable() {
  __enable_irq();
}

inline __attribute__((always_inline)) void interrupts_disable() {
  __disable_irq();
}

void init_gpio() {
  // Enable GPIO ports A, B, C & D
  RCC->IOPENR |= (RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN | RCC_IOPENR_GPIOCEN | RCC_IOPENR_GPIODEN);

  GPIO_SET_MODE(GPIOD, BIT_08_POS, MODER_OUT);  // Set LED status (PD8) to ouput
  GPIO_SET_MODE(GPIOC, BIT_06_POS, MODER_OUT);  // Set FAN 0 (PC6) to output
  GPIO_SET_MODE(GPIOB, BIT_15_POS, MODER_OUT);  // Set FAN 2 (PB15) to output
  GPIO_SET_MODE(GPIOC, BIT_08_POS, MODER_OUT);  // Set E0 heater (PC8) to output
}

void board_init_hal() {
  init_clock();

  init_gpio();

  // Init timers
  timer6_init();
  timer7_init(1000, true);
  timer14_init();

  set_timer7_interval(1000);

  // TMC2209 uart
  tmc2209_uart4_init();

  init_eeprom();
  i2c1_master_init();

  // Init USB
  usb_init_hal();
  tud_init(0);
}

void system_init_hal() {
  init_motion(0, 0, 0, 0);
}

void do_motion_planning() {
  // if (motion.steps_remaining > 0) {
  //   // No new motion while still moving
  //   return;
  // }

  // steppers_enable_hal(true);

  // if (motion.x.cur_pos == 0) {
  //   start_motion(1000, 1000, 1000, 1000);
  // } else {
  //   start_motion(0, 0, 0, 0);
  // }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void)remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
void cdc_task(void) {
  // Only check if connected
  if (tud_cdc_connected()) {
    if (tud_cdc_available()) {
      // Echo data
      char buf[64];
      uint32_t count = tud_cdc_read(buf, sizeof(buf));
      tud_cdc_write(buf, count);
      tud_cdc_write_flush();
    }
  }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  (void)itf;
  (void)rts;

  // TODO set some indicator
  if (dtr) {
    // Terminal connected
  } else {
    // Terminal disconnected
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf) {
  (void)itf;
  // while (tud_cdc_available()) {
  //   char c = tud_cdc_read_char();
  //   // Process character
  //   // You could echo it back:
  //   tud_cdc_write_char(c);
  //   tud_cdc_write_flush();
  // }
}

void hal_tick() {
  uint32_t elapsed_ms_count = get_systick();
  uint32_t elapsed_sec_count = get_second_counter();

  tud_task();  // Handle USB events
  cdc_task();

  // Run background tasks about every 100ms
  if (elapsed_ms_count % 100 == 0) {
    tmc2209_tick(elapsed_ms_count, elapsed_sec_count);
  }

  if (elapsed_ms_count % 2000 == 0) {
    do_motion_planning();
  }
}

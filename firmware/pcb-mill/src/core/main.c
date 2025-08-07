#include "hal.h"

#include "osal.h"
#include "usbd.h"
#include "cdc_device.h"

config_interface_t machine_config = {
    .version = 1 << 16  // Version 1.0
                        /* end default machine configuration*/
};

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
void tud_cdc_line_state_cb(bool dtr, bool rts) {
  (void)rts;

  // TODO set some indicator
  if (dtr) {
    // Terminal connected
  } else {
    // Terminal disconnected
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb() {
  // while (tud_cdc_available()) {
  //   char c = tud_cdc_read_char();
  //   // Process character
  //   // You could echo it back:
  //   tud_cdc_write_char(c);
  //   tud_cdc_write_flush();
  // }
}

void main() {
  // Initialise boards specific hardware
  board_init_hal();

  // Enable board interrupts
  interrupts_enable();

  // Initialise limit detection
  limits_init_hal();

  // Initialise USB
  usb_init_hal();
  tud_rhport_init(0);

  uint32_t config_version = config_get_version();

  if (config_version == CONFIG_INVALID_VERSION) {
    diag_print("Persisted configuration invalid, resetting configuration.\r\n");
    if (!config_reset(&machine_config)) {
      diag_print("Failed to persist configuration, halting device.\r\n");
      diag_flush();  // Make sure all diagnostic message is flushed
      device_halt();
    }
  }

#if DIAG_PRINT_SUPPORTED
  diag_printf("pcb mill v%d.%d\r\n", machine_config.version >> 16, machine_config.version & 0xFF);
#endif  // DIAG_PRINT_SUPPORTED

  // Loop forever
  while (true) {
    tud_task();  // Handle USB events
    cdc_task();
  }
}
#include "hal.h"
#include "cdc_device.h"

config_interface_t machine_config = {
    .version = 1 << 16  // Version 1.0

    /* end default machine configuration*/
};

void tud_cdc_line_state_cb(bool dtr, bool rts) {
  diag_printf("dtr: %d, rts: %d\r\n", dtr, rts);
}

void tud_cdc_rx_cb() {
  while (usb_cdc_available()) {
    // Echo data
    char c = usb_cdc_read_char();
    usb_cdc_write_char(c);
    usb_cdc_write_flush();
  }
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
  usb_init_driver();

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
  }
}
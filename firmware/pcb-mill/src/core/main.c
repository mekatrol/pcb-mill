#include "hal.h"

config_interface_t machine_config = {
    .version = 1 << 16  // Version 1.0
                        /* end default machine configuration*/
};

void main() {
  // Initialise boards specific hardware
  board_init_hal();

  // Enable board interrupts
  interrupts_enable();

  limits_init_hal();

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
  while (true);
}
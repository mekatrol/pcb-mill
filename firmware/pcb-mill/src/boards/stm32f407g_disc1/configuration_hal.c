#include "machine_config.h"

uint32_t config_get_version() { return 0; }

bool config_reset(config_interface_t* config) {
  (void)config;
  return false;
}
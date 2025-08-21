#ifndef __PERSISTENCE_H__
#define __PERSISTENCE_H__

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_INVALID_VERSION ~(0UL)  // 0xFFFF on 32bit device

typedef struct {
  uint32_t version;
} config_interface_t;

extern config_interface_t machine_config;

// Get the currently configured version, return CONFIG_INVALID_VERSION if no valid configuration
uint32_t config_get_version();

// Reset the configuration, will return true if successful
bool config_reset(config_interface_t* config);

#endif  // __PERSISTENCE_H__
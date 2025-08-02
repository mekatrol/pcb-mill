#include "machine_config.h"

#define PERSIST_KEY_HIGH 0xAA
#define PERSIST_KEY_LOW 0x55

uint8_t at24c32_read_byte(uint16_t mem_addr);
void at24c32_write_byte(uint16_t mem_addr, uint8_t data);

uint32_t config_get_version() {
  uint8_t key_high = at24c32_read_byte(0);
  uint8_t key_low = at24c32_read_byte(1);

  // The the expected config key is not correct then return invalid version
  // to signal that the eeprom needs to be reset
  if (key_high != PERSIST_KEY_HIGH || key_low != PERSIST_KEY_LOW) {
    return CONFIG_INVALID_VERSION;
  }

  // Read the version parts, verion is in form version_major.version_minor
  // With version being 32 bits, version major 16 bits and version minor 16 bits
  uint8_t ver_major_high = at24c32_read_byte(2);
  uint8_t ver_major_low = at24c32_read_byte(3);
  uint8_t ver_minor_high = at24c32_read_byte(4);
  uint8_t ver_minor_low = at24c32_read_byte(5);

  return ver_major_high << 24 | ver_major_low << 16 | ver_minor_high << 8 | ver_minor_low;
}

bool config_reset(config_interface_t* config) {
  // Write key
  at24c32_write_byte(0, PERSIST_KEY_HIGH);
  at24c32_write_byte(1, PERSIST_KEY_LOW);

  // Write version
  at24c32_write_byte(2, ((config->version >> 24) & 0xFF));
  at24c32_write_byte(3, ((config->version >> 16) & 0xFF));
  at24c32_write_byte(4, ((config->version >> 8) & 0xFF));
  at24c32_write_byte(5, ((config->version >> 0) & 0xFF));

  // Return true if successfully written
  return config_get_version() == config->version;
}
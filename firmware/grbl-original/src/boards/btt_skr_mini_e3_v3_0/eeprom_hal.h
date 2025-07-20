void init_eeprom();
void i2c1_master_init();
uint8_t at24c32_read_byte(uint16_t mem_addr);
void at24c32_write_byte(uint16_t mem_addr, uint8_t data);

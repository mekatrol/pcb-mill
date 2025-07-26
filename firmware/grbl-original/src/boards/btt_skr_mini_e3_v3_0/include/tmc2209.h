#ifndef __TMC2209_H__
#define __TMC2209_H__

#include <stdint.h>

void tmc2209_uart4_init();
void tmc2209_read_gconf(uint8_t slave);
int tmc2209_parse_reply(uint8_t sent_count, uint8_t *data_out);

#endif  // __TMC2209_H__
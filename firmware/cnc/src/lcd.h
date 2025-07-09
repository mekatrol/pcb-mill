#ifndef LCD_H
#define LCD_H

#include <stdint.h>

// === LCD Control Functions ===
void lcd_init(void);
void lcd_send_cmd(uint8_t cmd);
void lcd_send_data(uint8_t data);
void lcd_reset(void);
void lcd_clear(void);
void lcd_fill(uint8_t pattern);
void lcd_set_cursor(uint8_t page, uint8_t column);
void lcd_draw_char(char c, uint8_t page, uint8_t col);
void lcd_draw_string(const char *str, uint8_t page, uint8_t col);

#endif // LCD_H

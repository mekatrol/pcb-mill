#include <stdint.h>
#include "lcd.h"
#include "registers.h"

//                                   SKR       MINI      LPC1769   Function
//                                   -------   -------   -------   ---------------------------
#define LCD_SD_MISO_PIN (1 << 17) // EXP2_01   EXP2_10   P0.17     SD MISO
#define LCD_SD_SCK_PIN (1 << 15)  // EXP2_02   EXP2_09   P0.15     SD SCK
#define LCD_EN1_PIN (1 << 26)     // EXP2_03   EXP2_08   P3.26     Encoder EN1
#define LCD_UNUSED_PIN (1 << 16)  // EXP2_04   EXP2_07   P0.16     SD CS (CD/DAT3)
#define LCD_EN2_PIN (1 << 25)     // EXP2_05   EXP2_06   P3.25     Encoder EN2
#define LCD_SD_MOSI_PIN (1 << 18) // EXP2_06   EXP2_05   P0.18     SD MOSI
#define LCD_CD_PIN (1 << 31)      // EXP2_07   EXP2_04   P1.31     SD Card detect
#define LCD_BTN_RST -1            // EXP2_08   EXP2_03   N/A       LPC1769 Reset
#define LCD_GND -1                // EXP2_09   EXP2_02   N/A       GND
#define LCD_KILL -1               // EXP2_10   EXP2_01   N/A       KILL
#define LCD_BEEP_PIN (1 << 30)    // EXP1_01   EXP1_10   P1.30     Beep
#define LCD_ENC_PIN (1 << 28)     // EXP1_02   EXP1_09   P0.28     Encoder button
#define LCD_SS_PIN (1 << 18)      // EXP1_03   EXP1_08   P1.18     Slave select (LCD enable)
#define LCD_DC_PIN (1 << 19)      // EXP1_04   EXP1_07   P1.19     Data/Command (A0)
#define LCD_RST_PIN (1 << 20)     // EXP1_05   EXP1_06   P1.20     LCD Reset
#define LCD_RED_PIN (1 << 21)     // EXP1_06   EXP1_05   P1.21     Red (neopixel)
#define LCD_GREEN_PIN (1 << 22)   // EXP1_07   EXP1_04   P1.22     Green
#define LCD_BLUE_PIN (1 << 23)    // EXP1_08   EXP1_03   P1.23     Blue
#define LCD_GND -1                // EXP1_09   EXP2_02   N/A       GND
#define LCD_KILL -1               // EXP1_10   EXP2_01   N/A       VCC

// === Font: 5x7 ASCII font (partial) ===
static const uint8_t font5x7[][5] = {
    // Only characters 32 to 127
    // Each character is 5 bytes wide
    [0] = {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    [1] = {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    [2] = {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    [3] = {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    [4] = {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    [5] = {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    [6] = {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    [7] = {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    [8] = {0x1C, 0x22, 0x41, 0x00, 0x00}, // (
    [9] = {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    // ... (you can expand this or load from PROGMEM/ROM)
};

static void spi_init(void)
{
    PINSEL0 |= (2 << 30); // P0.15 = SCK
    PINSEL1 |= (2 << 2);  // P0.18 = MOSI0 (SPI0 MOSI)

    // With F_CPU = 120 MHz and S0SPCCR = 12:
    // SPI Clock=
    //      12
    //    -------
    //    120 MHz
    //    = 10 MHz
    S0SPCCR = 12;

    S0SPCR = (1 << 5); // Enable SPI in master mode
}

void lcd_reset(void)
{
    // Toggle LCD reset low
    GPIO1_CLR = LCD_RST_PIN;
    for (volatile int i = 0; i < 10000; i++)
        ;
    GPIO1_SET = LCD_RST_PIN;
}

static void spi_send(uint8_t data)
{
    S0SPDR = data;
    while (!(S0SPSR & (1 << 7)))
        ; // Wait for transfer complete
}

void lcd_send_cmd(uint8_t cmd)
{
    GPIO1_CLR = LCD_DC_PIN;
    GPIO0_CLR = LCD_SS_PIN;
    spi_send(cmd);
    GPIO0_SET = LCD_SS_PIN;
}

void lcd_send_data(uint8_t data)
{
    GPIO1_SET = LCD_DC_PIN;
    GPIO0_CLR = LCD_SS_PIN;
    spi_send(data);
    GPIO0_SET = LCD_SS_PIN;
}

void lcd_init(void)
{
    // Set slave select to output
    GPIO0_DIR |= LCD_SS_PIN;

    // Set data/command and reset to output
    GPIO1_DIR |= LCD_DC_PIN | LCD_RST_PIN;

    spi_init();
    lcd_reset();

    lcd_send_cmd(0xAE); // Display off
    lcd_send_cmd(0xA2); // Bias
    lcd_send_cmd(0xA0); // Normal
    lcd_send_cmd(0xC8); // COM scan
    lcd_send_cmd(0x22); // Resistor ratio
    lcd_send_cmd(0x2F); // Power
    lcd_send_cmd(0x40); // Start line
    lcd_send_cmd(0xAF); // Display on
}

void lcd_set_cursor(uint8_t page, uint8_t column)
{
    lcd_send_cmd(0xB0 | (page & 0x0F));          // Page address
    lcd_send_cmd(0x10 | ((column >> 4) & 0x0F)); // Column MSB
    lcd_send_cmd(0x00 | (column & 0x0F));        // Column LSB
}

void lcd_clear(void)
{
    for (uint8_t page = 0; page < 8; page++)
    {
        lcd_set_cursor(page, 0);
        for (uint8_t col = 0; col < 128; col++)
            lcd_send_data(0x00);
    }
}

void lcd_fill(uint8_t pattern)
{
    for (uint8_t page = 0; page < 8; page++)
    {
        lcd_set_cursor(page, 0);
        for (uint8_t col = 0; col < 128; col++)
            lcd_send_data(pattern);
    }
}

void lcd_draw_char(char c, uint8_t page, uint8_t col)
{
    if (c < 32 || c > 126)
        c = '?';

    lcd_set_cursor(page, col);

    const uint8_t *glyph = font5x7[c - 32];
    for (int i = 0; i < 5; i++)
        lcd_send_data(glyph[i]);

    lcd_send_data(0x00); // Space between characters
}

void lcd_draw_string(const char *str, uint8_t page, uint8_t col)
{
    while (*str && col < 123) // Leave room for spacing
    {
        lcd_draw_char(*str++, page, col);
        col += 6; // 5px character + 1px space
    }
}

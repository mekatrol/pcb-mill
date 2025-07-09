#include <stdint.h>
#include "lcd.h"
#include "neopixel.h"
#include "pwm.h"
#include "registers.h"
#include "systick.h"
#include "uart.h"

#define GPIO2_PINS ((1 << 3) | (1 << 4) | (1 << 5) | (1 << 7))
#define LCD_RED_PIN (1 << 21)   // P1.21
#define LCD_GREEN_PIN (1 << 22) // P1.22
#define LCD_BLUE_PIN (1 << 23)  // P1.23

int main(void)
{
    clock_init();
    systick_init();
    uart_init(115200);

    lcd_init();
    lcd_clear();
    lcd_draw_string("FYSETC Mini12864", 0, 0);
    lcd_draw_string("SKR 1.4 Turbo", 1, 0);

    // pwm_init();
    // pwm_set_rgb(0, 255, 0); // Green

    uart_send_str("UART0 ready!\r\n");

    neopixel_init();
    neopixel_send_color(255, 0, 0);

    GPIO2->DIR |= GPIO2_PINS;

    // Fill screen (just a test pattern)
    for (int page = 0; page < 8; page++)
    {
        lcd_send_cmd(0xB0 | page); // Page address
        lcd_send_cmd(0x10);        // Column MSB
        lcd_send_cmd(0x00);        // Column LSB
        for (int i = 0; i < 128; i++)
        {
            lcd_send_data(0xAA); // Test pattern
        }
    }

    while (1)
    {
        uart_send_str("Toggling GPIOs...\r\n");

        GPIO2->SET = GPIO2_PINS;
        delay_ms(1000);

        GPIO2->CLR = GPIO2_PINS;
        delay_ms(1000);
    }

    // Should never return
    return -1;
}

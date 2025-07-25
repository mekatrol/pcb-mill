#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

extern void serial_write(uint8_t data);

void uart_puts(const char *s) {
  while (*s) {
    serial_write(*s++);
  }
}

static void uart_putnum(unsigned int num, unsigned int base, bool uppercase) {
  char buf[32];
  int i = 0;
  const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

  if (num == 0) {
    serial_write('0');
    return;
  }

  while (num) {
    buf[i++] = digits[num % base];
    num /= base;
  }

  while (--i >= 0) {
    serial_write(buf[i]);
  }
}

void uart_printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  while (*fmt) {
    if (*fmt != '%') {
      serial_write(*fmt++);
      continue;
    }

    fmt++;  // skip '%'

    switch (*fmt++) {
      case 'c': {
        char c = (char)va_arg(args, int);
        serial_write(c);
        break;
      }
      case 's': {
        char *s = va_arg(args, char *);
        uart_puts(s);
        break;
      }
      case 'd': {
        int n = va_arg(args, int);
        if (n < 0) {
          serial_write('-');
          n = -n;
        }
        uart_putnum((unsigned int)n, 10, false);
        break;
      }
      case 'x': {
        unsigned int n = va_arg(args, unsigned int);
        uart_putnum(n, 16, false);
        break;
      }
      case '%': {
        serial_write('%');
        break;
      }
      default:
        // Unsupported format, just print it raw
        serial_write(*(fmt - 1));
        break;
    }
  }

  va_end(args);
}

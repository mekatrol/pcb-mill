#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "hal.h"
#include "diagnostics.h"

static void diag_putnum(unsigned int num, unsigned int base, bool uppercase);

void diag_print(const char *s) {
  while (*s) {
    diag_send(*s++);
  }
}

void diag_printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  while (*fmt) {
    if (*fmt != '%') {
      diag_send(*fmt++);
      continue;
    }

    fmt++;  // skip '%'

    switch (*fmt++) {
      case 'c': {
        char c = (char)va_arg(args, int);
        diag_send(c);
        break;
      }
      case 's': {
        char *s = va_arg(args, char *);
        diag_print(s);
        break;
      }
      case 'd': {
        int n = va_arg(args, int);
        if (n < 0) {
          diag_send('-');
          n = -n;
        }
        diag_putnum((unsigned int)n, 10, false);
        break;
      }
      case 'x': {
        unsigned int n = va_arg(args, unsigned int);
        diag_putnum(n, 16, false);
        break;
      }
      case '%': {
        diag_send('%');
        break;
      }
      default:
        // Unsupported format, just print it raw
        diag_send(*(fmt - 1));
        break;
    }
  }

  va_end(args);
}

static void diag_putnum(unsigned int num, unsigned int base, bool uppercase) {
  char buf[32];
  int i = 0;
  const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

  if (num == 0) {
    diag_send('0');
    return;
  }

  while (num) {
    buf[i++] = digits[num % base];
    num /= base;
  }

  while (--i >= 0) {
    diag_send(buf[i]);
  }
}

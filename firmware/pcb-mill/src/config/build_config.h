#ifndef __CONFIG_H__
#define __CONFIG_H__

// The default baud rate used for any communication
#define BAUD_RATE 115200

// If the HAL supports printing diagnostic messages then set DIAG_PRINT_SUPPORTED to non-zero
#define DIAG_PRINT_SUPPORTED true
#if DIAG_PRINT_SUPPORTED

void diag_flush();                       // Wait for all current diag message characters to finish
void diag_print(const char *s);          // Write diagnostic string
void diag_printf(const char *fmt, ...);  // Write formatted diagnostic string

#endif  // DIAG_PRINT_SUPPORTED

#endif  // __CONFIG_H__
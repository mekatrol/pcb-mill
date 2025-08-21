#ifndef __DIAGNOSTICS_H__
#define __DIAGNOSTICS_H__

#include "build_config.h"

#if DIAG_PRINT_SUPPORTED

void diag_send(uint8_t b);
void diag_flush();                       // Wait for all current diag message characters to finish
void diag_print(const char *s);          // Write diagnostic string
void diag_printf(const char *fmt, ...);  // Write formatted diagnostic string

#endif  // DIAG_PRINT_SUPPORTED

#endif  // __DIAGNOSTICS_H__
/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#ifndef __AVR__
#ifndef __PGMSPACE_H_
// This define should prevent reading the system pgmspace.h if included elsewhere
// This is not normally needed.
#define __PGMSPACE_H_ 1
#endif

#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef PGM_P
#define PGM_P  const char *
#endif
#ifndef PSTR
#define PSTR(str) (str)
#endif
#ifndef F
class __FlashStringHelper;
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))
#endif
#ifndef _SFR_BYTE
#define _SFR_BYTE(n) (n)
#endif
#ifndef memchr_P
#define memchr_P(str, c, len) memchr((str), (c), (len))
#endif
#ifndef memcmp_P
#define memcmp_P(a, b, n) memcmp((a), (b), (n))
#endif
#ifndef memcpy_P
#define memcpy_P(dest, src, num) memcpy((dest), (src), (num))
#endif
#ifndef memmem_P
#define memmem_P(a, alen, b, blen) memmem((a), (alen), (b), (blen))
#endif
#ifndef memrchr_P
#define memrchr_P(str, val, len) memrchr((str), (val), (len))
#endif
#ifndef strcat_P
#define strcat_P(dest, src) strcat((dest), (src))
#endif
#ifndef strchr_P
#define strchr_P(str, c) strchr((str), (c))
#endif
#ifndef strchrnul_P
#define strchrnul_P(str, c) strchrnul((str), (c))
#endif
#ifndef strcmp_P
#define strcmp_P(a, b) strcmp((a), (b))
#endif
#ifndef strcpy_P
#define strcpy_P(dest, src) strcpy((dest), (src))
#endif
#ifndef strcasecmp_P
#define strcasecmp_P(a, b) strcasecmp((a), (b))
#endif
#ifndef strcasestr_P
#define strcasestr_P(a, b) strcasestr((a), (b))
#endif
#ifndef strlcat_P
#define strlcat_P(dest, src, len) strlcat((dest), (src), (len))
#endif
#ifndef strlcpy_P
#define strlcpy_P(dest, src, len) strlcpy((dest), (src), (len))
#endif
#ifndef strlen_P
#define strlen_P(s) strlen((const char *)(s))
#endif
#ifndef strnlen_P
#define strnlen_P(str, len) strnlen((str), (len))
#endif
#ifndef strncmp_P
#define strncmp_P(a, b, n) strncmp((a), (b), (n))
#endif
#ifndef strncasecmp_P
#define strncasecmp_P(a, b, n) strncasecmp((a), (b), (n))
#endif
#ifndef strncat_P
#define strncat_P(a, b, n) strncat((a), (b), (n))
#endif
#ifndef strncpy_P
#define strncpy_P(a, b, n) strncpy((a), (b), (n))
#endif
#ifndef strpbrk_P
#define strpbrk_P(str, chrs) strpbrk((str), (chrs))
#endif
#ifndef strrchr_P
#define strrchr_P(str, c) strrchr((str), (c))
#endif
#ifndef strsep_P
#define strsep_P(pstr, delim) strsep((pstr), (delim))
#endif
#ifndef strspn_P
#define strspn_P(str, chrs) strspn((str), (chrs))
#endif
#ifndef strstr_P
#define strstr_P(a, b) strstr((a), (b))
#endif
#ifndef sprintf_P
#define sprintf_P(s, ...) sprintf((s), __VA_ARGS__)
#endif
#ifndef vfprintf_P
#define vfprintf_P(s, ...) vfprintf((s), __VA_ARGS__)
#endif
#ifndef printf_P
#define printf_P(...) printf(__VA_ARGS__)
#endif
#ifndef snprintf_P
#define snprintf_P(s, n, ...) snprintf((s), (n), __VA_ARGS__)
#endif
#ifndef vsprintf_P
#define vsprintf_P(s, ...) vsprintf((s),__VA_ARGS__)
#endif
#ifndef vsnprintf_P
#define vsnprintf_P(s, n, ...) vsnprintf((s), (n),__VA_ARGS__)
#endif
#ifndef fprintf_P
#define fprintf_P(s, ...) fprintf((s), __VA_ARGS__)
#endif

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#endif
#ifndef pgm_read_float
#define pgm_read_float(addr) (*(const float *)(addr))
#endif

#ifndef pgm_read_byte_near
#define pgm_read_byte_near(addr) pgm_read_byte(addr)
#endif
#ifndef pgm_read_word_near
#define pgm_read_word_near(addr) pgm_read_word(addr)
#endif
#ifndef pgm_read_dword_near
#define pgm_read_dword_near(addr) pgm_read_dword(addr)
#endif
#ifndef pgm_read_float_near
#define pgm_read_float_near(addr) pgm_read_float(addr)
#endif
#ifndef pgm_read_byte_far
#define pgm_read_byte_far(addr) pgm_read_byte(addr)
#endif
#ifndef pgm_read_word_far
#define pgm_read_word_far(addr) pgm_read_word(addr)
#endif
#ifndef pgm_read_dword_far
#define pgm_read_dword_far(addr) pgm_read_dword(addr)
#endif
#ifndef pgm_read_float_far
#define pgm_read_float_far(addr) pgm_read_float(addr)
#endif

#ifndef pgm_read_pointer
#define pgm_read_pointer
#endif

// Fix bug in pgm_read_ptr
#undef pgm_read_ptr
#define pgm_read_ptr(addr) (*((void**)(addr)))

#endif // __AVR__

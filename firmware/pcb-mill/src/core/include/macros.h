#ifndef __MACROS_H__
#define __MACROS_H__

#define ALWAYS_INLINE __attribute__((always_inline)) inline

// On small MCUs without a hardware divider (like STM32G0, Cortex-M0+), % (modulus) can take dozens of cycles, but & (bit and) is 1 cycle
// so use power of two calculation to determine if a % b == 0
// (a % b) == (a & (b - 1))
// IMPORTANT: b must be a power of 2!
#define IS_MULTIPLE_POW2(a, b) ((((a) & ((b) - 1))) == 0)

#endif  // __MACROS_H__
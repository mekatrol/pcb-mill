#ifndef __ALIGNED_MEMORY_H__
#define __ALIGNED_MEMORY_H__

#include <stdint.h>

/**************************************************************************************************************************************************
 *
 * 1. ALIGNED MEMORY ACCESS
 *
 *     Access is aligned if the address of the data is a multiple of its size being accessed.
 *
 *     Examples:
 *         uint8_t (1 byte) → can be at any address.
 *         uint16_t (2 bytes) → must be at even addresses (0x20000000, 0x20000002, …).
 *         uint32_t (4 bytes) → must be at addresses multiple of 4 (0x20000000, 0x20000004, …).
 *         uint64_t (8 bytes) → must be at addresses multiple of 8.
 *
 *     If you follow these rules on most processors then memory accesses are single, efficient instructions (LDR, STR, etc.) with no penalty.
 *
 * 2. UNALIGNED MEMORY ACCESS
 *
 *     Access is unaligned if the address is not a multiple of the data size being accessed.
 *
 *     Example:
 *         uint8_t buf[4];
 *         uint32_t *p = (uint32_t*)&buf[1];  // misaligned by 1
 *         uint32_t x = *p;  // unaligned access
 *
 *     The pointer p points to an address not divisible by 4.
 *     This means the CPU must fetch a word starting at a misaligned address.
 *
 * 3. Example: STM32G0B1 (Cortex-M0+)
 *
 *     Unaligned access is NOT supported in hardware.
 *     Attempting it will trigger a HardFault exception (HARDFAULT → UNALIGNED).
 *     This is unlike Cortex-M3/M4/M7, which can do unaligned access in hardware (though probably slower than aligned access).
 *
 *     So, on STM32G0B1:
 *         uint32_t *p = (uint32_t*)0x20000001;
 *         uint32_t val = *p;
 *     → Causes HardFault.
 *
 * 4. Why does this matter?
 *
 *     Structures:
 *          If you __attribute__((packed)) a struct, fields may not be aligned. Directly accessing them as words may fault.
 *
 *     Casting:
 *          Casting a uint8_t* buffer to a uint32_t* can fault unless the buffer is 4-byte aligned.
 *
 *     USB / networking protocols:
 *          Protocol headers are often packed, so you must take care when reading multi-byte fields.
 *
 * 5. Solutions / Safe Approaches
 *
 *     Compiler alignment: By default, GCC aligns struct members properly (unless you use __packed__).
 *
 *     Read/write one byte at a time, then reconstruct:
 *
 *     static inline uint32_t read_u32_unaligned(const void *p) {
 *         uint8_t const *b = (uint8_t const*)p;
 *         return (uint32_t)b[0] |
 *             ((uint32_t)b[1] << 8) |
 *             ((uint32_t)b[2] << 16) |
 *             ((uint32_t)b[3] << 24);
 *     }
 *
 *     Similarly for 16-bit.
 *
 *     Use memcpy(): GCC will optimize memcpy of small fixed sizes into byte loads/stores.
 *
 *         uint32_t val;
 *         memcpy(&val, p, sizeof(val));
 *
 *     This avoids alignment issues because it copies byte by byte.
 *
 * 6. Performance impact
 *
 *     Aligned access → single instruction, fastest possible.
 *
 *     Unaligned access:
 *
 *         On M0+ → HardFault (must avoid!).
 *         On M3/M4/M7 → allowed but slower (can take multiple cycles).
 **************************************************************************************************************************************************/

ALWAYS_INLINE static uint16_t unaligned_read_16(const uint8_t* p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

ALWAYS_INLINE static void unaligned_write_16(uint8_t* p, uint16_t value) {
  p[0] = (uint8_t)(value);
  p[1] = (uint8_t)(value >> 8);
}

ALWAYS_INLINE static uint32_t unaligned_read_32(const uint8_t* p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

ALWAYS_INLINE static void unaligned_write_32(uint8_t* p, uint32_t value) {
  p[0] = (uint8_t)(value);
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
}

#endif  // __ALIGNED_MEMORY_H__
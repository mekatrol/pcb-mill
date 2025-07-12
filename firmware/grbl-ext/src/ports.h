#ifndef __PORTS_H__
#define __PORTS_H__

/*
 * MODER = Port mode register
 */
#define MODER_BIT_COUNT 0x02UL // 2 bits per MODER port configuration
#define MODER_MSK 0x03UL
#define MODER_INP 0x00UL
#define MODER_OUT 0x01UL
#define MODER_ALT 0x02UL
#define MODER_ANA 0x03UL

#define MODE_00 0x00
#define MODE_01 0x01
#define MODE_02 0x02
#define MODE_03 0x03
#define MODE_04 0x04
#define MODE_05 0x05
#define MODE_06 0x06
#define MODE_07 0x07
#define MODE_08 0x08
#define MODE_09 0x09
#define MODE_10 0x10
#define MODE_11 0x11
#define MODE_12 0x12
#define MODE_13 0x13
#define MODE_14 0x14
#define MODE_15 0x15

#define IOPENR_PORTA_ENABLE (1 << 0)
#define IOPENR_PORTB_ENABLE (1 << 1)
#define IOPENR_PORTC_ENABLE (1 << 2)
#define IOPENR_PORTD_ENABLE (1 << 3)
#define IOPENR_PORTE_ENABLE (1 << 4)
#define IOPENR_PORTF_ENABLE (1 << 5)

#endif // __PORTS_H__
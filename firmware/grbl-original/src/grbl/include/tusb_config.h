#ifndef __TUSB_CONFIG_H__
#define __TUSB_CONFIG_H__

#define CFG_TUSB_MCU OPT_MCU_STM32G0
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

#define CFG_TUSB_OS OPT_OS_NONE
#define CFG_TUSB_DEBUG 0

#define CFG_TUD_ENDPOINT0_SIZE 64

// Enable CDC only
#define CFG_TUD_CDC 1
#define CFG_TUD_MSC 0
#define CFG_TUD_HID 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0

// CDC buffer sizes
#define CFG_TUD_CDC_RX_BUFSIZE 64
#define CFG_TUD_CDC_TX_BUFSIZE 64

#endif  // __TUSB_CONFIG_H__

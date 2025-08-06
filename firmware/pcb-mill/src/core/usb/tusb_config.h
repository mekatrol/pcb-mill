#ifndef __TUSB_CONFIG_H__
#define __TUSB_CONFIG_H__

#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

#define CFG_TUD_ENDPOINT0_SIZE 64

// CDC buffer sizes
#define CFG_TUD_CDC_RX_BUFSIZE 64
#define CFG_TUD_CDC_TX_BUFSIZE 64

#endif  // __TUSB_CONFIG_H__

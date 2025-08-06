#ifndef _TUSB_OPTION_H_
#define _TUSB_OPTION_H_

#include "tusb_compiler.h"

// Version is release as major.minor.revision eg 1.0.0
#define TUSB_VERSION_MAJOR 0
#define TUSB_VERSION_MINOR 18
#define TUSB_VERSION_REVISION 0

//--------------------------------------------------------------------+
// Mode and Speed
//--------------------------------------------------------------------+

// Low byte is operational mode
#define OPT_MODE_NONE 0x0000    ///< Disabled
#define OPT_MODE_DEVICE 0x0001  ///< Device Mode
#define OPT_MODE_HOST 0x0002    ///< Host Mode

// High byte is max operational speed (corresponding to tusb_speed_t)
#define OPT_MODE_DEFAULT_SPEED 0x0000  ///< Default (max) speed supported by MCU
#define OPT_MODE_LOW_SPEED 0x0100      ///< Low Speed
#define OPT_MODE_FULL_SPEED 0x0200     ///< Full Speed
#define OPT_MODE_HIGH_SPEED 0x0400     ///< High Speed
#define OPT_MODE_SPEED_MASK 0xff00

#include "usb.h"

//--------------------------------------------------------------------
// RootHub Mode detection
//--------------------------------------------------------------------

//------------- Root hub as Device -------------//

#define TUD_RHPORT_MODE (CFG_TUSB_RHPORT0_MODE)
#define TUD_OPT_RHPORT 0

#ifndef CFG_TUD_ENABLED
// fallback to use CFG_TUSB_RHPORTx_MODE
#define CFG_TUD_ENABLED (TUD_RHPORT_MODE & OPT_MODE_DEVICE)
#endif

#ifndef CFG_TUD_MAX_SPEED
// fallback to use CFG_TUSB_RHPORTx_MODE
#define CFG_TUD_MAX_SPEED (TUD_RHPORT_MODE & OPT_MODE_SPEED_MASK)
#endif

// For backward compatible
#define TUSB_OPT_DEVICE_ENABLED CFG_TUD_ENABLED

// highspeed support indicator
#define TUD_OPT_HIGH_SPEED (CFG_TUD_MAX_SPEED & OPT_MODE_HIGH_SPEED)

//------------- Root hub as Host -------------//

#define TUH_RHPORT_MODE OPT_MODE_NONE

#ifndef CFG_TUH_ENABLED
// fallback to use CFG_TUSB_RHPORTx_MODE
#define CFG_TUH_ENABLED (TUH_RHPORT_MODE & OPT_MODE_HOST)
#endif

#ifndef CFG_TUH_MAX_SPEED
// fallback to use CFG_TUSB_RHPORTx_MODE
#define CFG_TUH_MAX_SPEED (TUH_RHPORT_MODE & OPT_MODE_SPEED_MASK)
#endif

// For backward compatible
#define TUSB_OPT_HOST_ENABLED CFG_TUH_ENABLED

// highspeed support indicator
#define TUH_OPT_HIGH_SPEED (CFG_TUH_MAX_SPEED & OPT_MODE_HIGH_SPEED)

//--------------------------------------------------------------------+
// Common Options (Default)
//--------------------------------------------------------------------+

// Level where CFG_TUSB_DEBUG must be at least for USBH is logged
#ifndef CFG_TUH_LOG_LEVEL
#define CFG_TUH_LOG_LEVEL 2
#endif

// Level where CFG_TUSB_DEBUG must be at least for USBD is logged
#ifndef CFG_TUD_LOG_LEVEL
#define CFG_TUD_LOG_LEVEL 2
#endif

// Memory section for placing buffer used for usb transferring. If MEM_SECTION is different for
// host and device use: CFG_TUD_MEM_SECTION, CFG_TUH_MEM_SECTION instead
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

// Alignment requirement of buffer used for usb transferring. if MEM_ALIGN is different for
// host and device controller use: CFG_TUD_MEM_ALIGN, CFG_TUH_MEM_ALIGN instead
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

#ifndef CFG_TUSB_MEM_DCACHE_LINE_SIZE
#ifndef CFG_TUSB_MEM_DCACHE_LINE_SIZE_DEFAULT
#define CFG_TUSB_MEM_DCACHE_LINE_SIZE_DEFAULT 1
#endif

#define CFG_TUSB_MEM_DCACHE_LINE_SIZE CFG_TUSB_MEM_DCACHE_LINE_SIZE_DEFAULT
#endif

#ifndef CFG_TUSB_OS_INC_PATH
#ifndef CFG_TUSB_OS_INC_PATH_DEFAULT
#define CFG_TUSB_OS_INC_PATH_DEFAULT
#endif

#define CFG_TUSB_OS_INC_PATH CFG_TUSB_OS_INC_PATH_DEFAULT
#endif

//--------------------------------------------------------------------
// Device Options (Default)
//--------------------------------------------------------------------

// Attribute to place data in accessible RAM for device controller (default: CFG_TUSB_MEM_SECTION)
#ifndef CFG_TUD_MEM_SECTION
#define CFG_TUD_MEM_SECTION CFG_TUSB_MEM_SECTION
#endif

// Attribute to align memory for device controller (default: CFG_TUSB_MEM_ALIGN)
#ifndef CFG_TUD_MEM_ALIGN
#define CFG_TUD_MEM_ALIGN CFG_TUSB_MEM_ALIGN
#endif

#ifndef CFG_TUD_MEM_DCACHE_ENABLE
#ifndef CFG_TUD_MEM_DCACHE_ENABLE_DEFAULT
#define CFG_TUD_MEM_DCACHE_ENABLE_DEFAULT 0
#endif

#define CFG_TUD_MEM_DCACHE_ENABLE CFG_TUD_MEM_DCACHE_ENABLE_DEFAULT
#endif

#ifndef CFG_TUD_MEM_DCACHE_LINE_SIZE
#define CFG_TUD_MEM_DCACHE_LINE_SIZE CFG_TUSB_MEM_DCACHE_LINE_SIZE
#endif

#ifndef CFG_TUD_INTERFACE_MAX
#define CFG_TUD_INTERFACE_MAX 16
#endif

// default to max hardware endpoint, but can be smaller to save RAM
#ifndef CFG_TUD_ENDPPOINT_MAX
#define CFG_TUD_ENDPPOINT_MAX 8
#endif

// USB 2.0 7.1.20: compliance test mode support
#ifndef CFG_TUD_TEST_MODE
#define CFG_TUD_TEST_MODE 0
#endif

//------------- Device Class Driver -------------//
#ifndef CFG_TUD_BTH
#define CFG_TUD_BTH 0
#endif

#ifndef CFG_TUD_AUDIO
#define CFG_TUD_AUDIO 0
#endif

#ifndef CFG_TUD_VIDEO
#define CFG_TUD_VIDEO 0
#endif

#ifndef CFG_TUD_USBTMC
#define CFG_TUD_USBTMC 0
#endif

#ifndef CFG_TUD_DFU_RUNTIME
#define CFG_TUD_DFU_RUNTIME 0
#endif

#ifndef CFG_TUD_DFU
#define CFG_TUD_DFU 0
#endif

#define CFG_TUD_ECM_RNDIS 0

#ifndef CFG_TUD_NCM
#define CFG_TUD_NCM 0
#endif

// Attribute to place data in accessible RAM for host controller (default: CFG_TUSB_MEM_SECTION)
#ifndef CFG_TUH_MEM_SECTION
#define CFG_TUH_MEM_SECTION CFG_TUSB_MEM_SECTION
#endif

// Attribute to align memory for host controller
#ifndef CFG_TUH_MEM_ALIGN
#define CFG_TUH_MEM_ALIGN CFG_TUSB_MEM_ALIGN
#endif

#ifndef CFG_TUH_MEM_DCACHE_ENABLE
#ifndef CFG_TUH_MEM_DCACHE_ENABLE_DEFAULT
#define CFG_TUH_MEM_DCACHE_ENABLE_DEFAULT 0
#endif

#define CFG_TUH_MEM_DCACHE_ENABLE CFG_TUH_MEM_DCACHE_ENABLE_DEFAULT
#endif

#ifndef CFG_TUH_MEM_DCACHE_LINE_SIZE
#define CFG_TUH_MEM_DCACHE_LINE_SIZE CFG_TUSB_MEM_DCACHE_LINE_SIZE
#endif

//------------- CLASS -------------//

#ifndef CFG_TUH_HUB
#define CFG_TUH_HUB 0
#endif

#ifndef CFG_TUH_CDC
#define CFG_TUH_CDC 0
#endif

// FTDI is not part of CDC class, only to re-use CDC driver API
#ifndef CFG_TUH_CDC_FTDI
#define CFG_TUH_CDC_FTDI 0
#endif

// List of product IDs that can use the FTDI CDC driver. 0x0403 is FTDI's VID
#ifndef CFG_TUH_CDC_FTDI_VID_PID_LIST
#define CFG_TUH_CDC_FTDI_VID_PID_LIST                                       \
  {0x0403, 0x6001},     /* Similar device to SIO above */                   \
      {0x0403, 0x6006}, /* FTDI's alternate PID for above */                \
      {0x0403, 0x6010}, /* Dual channel device */                           \
      {0x0403, 0x6011}, /* Quad channel hi-speed device */                  \
      {0x0403, 0x6014}, /* Single channel hi-speed device */                \
      {0x0403, 0x6015}, /* FT-X series (FT201X, FT230X, FT231X, etc) */     \
      {0x0403, 0x6040}, /* Dual channel hi-speed device with PD */          \
      {0x0403, 0x6041}, /* Quad channel hi-speed device with PD */          \
      {0x0403, 0x6042}, /* Dual channel hi-speed device with PD */          \
      {0x0403, 0x6043}, /* Quad channel hi-speed device with PD */          \
      {0x0403, 0x6044}, /* Dual channel hi-speed device with PD */          \
      {0x0403, 0x6045}, /* Dual channel hi-speed device with PD */          \
      {0x0403, 0x6048}, /* Quad channel automotive grade hi-speed device */ \
      {0x0403, 0x8372}, /* Product Id SIO application of 8U100AX */         \
      {0x0403, 0xFBFA}, /* Product ID for FT232RL */                        \
      {0x0403, 0xCD18}, /* ??? */
#endif

// CP210X is not part of CDC class, only to re-use CDC driver API
#ifndef CFG_TUH_CDC_CP210X
#define CFG_TUH_CDC_CP210X 0
#endif

// List of product IDs that can use the CP210X CDC driver. 0x10C4 is Silicon Labs' VID
#ifndef CFG_TUH_CDC_CP210X_VID_PID_LIST
#define CFG_TUH_CDC_CP210X_VID_PID_LIST                    \
  {0x10C4, 0xEA60},     /* Silicon Labs factory default */ \
      {0x10C4, 0xEA61}, /* Silicon Labs factory default */ \
      {0x10C4, 0xEA70}  /* Silicon Labs Dual Port factory default */
#endif

#ifndef CFG_TUH_CDC_CH34X
// CH34X is not part of CDC class, only to re-use CDC driver API
#define CFG_TUH_CDC_CH34X 0
#endif

// List of product IDs that can use the CH34X CDC driver
#ifndef CFG_TUH_CDC_CH34X_VID_PID_LIST
#define CFG_TUH_CDC_CH34X_VID_PID_LIST                                                       \
  {0x1a86, 0x5523},     /* ch341 chip */                                                     \
      {0x1a86, 0x7522}, /* ch340k chip */                                                    \
      {0x1a86, 0x7523}, /* ch340 chip */                                                     \
      {0x1a86, 0xe523}, /* ch330 chip */                                                     \
      {0x4348, 0x5523}, /* ch340 custom chip */                                              \
      {0x2184, 0x0057}, /* overtaken from Linux Kernel driver /drivers/usb/serial/ch341.c */ \
      {0x9986, 0x7523}  /* overtaken from Linux Kernel driver /drivers/usb/serial/ch341.c */
#endif

#ifndef CFG_TUH_CDC_PL2303
// PL2303 is not part of CDC class, only to re-use CDC driver API
#define CFG_TUH_CDC_PL2303 0
#endif

#ifndef CFG_TUH_CDC_PL2303_VID_PID_QUIRKS_LIST
// List of product IDs that can use the PL2303 CDC driver
#define CFG_TUH_CDC_PL2303_VID_PID_LIST    \
  {0x067b, 0x2303},     /* initial 2303 */ \
      {0x067b, 0x2304}, /* TB */           \
      {0x067b, 0x23a3}, /* GC */           \
      {0x067b, 0x23b3}, /* GB */           \
      {0x067b, 0x23c3}, /* GT */           \
      {0x067b, 0x23d3}, /* GL */           \
      {0x067b, 0x23e3}, /* GE */           \
      {0x067b, 0x23f3}  /* GS */
#endif

#ifndef CFG_TUH_HID
#define CFG_TUH_HID 0
#endif

#ifndef CFG_TUH_MIDI
#define CFG_TUH_MIDI 0
#endif

#ifndef CFG_TUH_MSC
#define CFG_TUH_MSC 0
#endif

#ifndef CFG_TUH_VENDOR
#define CFG_TUH_VENDOR 0
#endif

#ifndef CFG_TUH_API_EDPT_XFER
#define CFG_TUH_API_EDPT_XFER 0
#endif

//--------------------------------------------------------------------+
// TypeC Options (Default)
//--------------------------------------------------------------------+

#ifndef CFG_TUC_ENABLED
#define CFG_TUC_ENABLED 0

#define tuc_int_handler(_p)
#endif

// To avoid GCC compiler warnings when -pedantic option is used (strict ISO C)
typedef int make_iso_compilers_happy;

#endif /* _TUSB_OPTION_H_ */

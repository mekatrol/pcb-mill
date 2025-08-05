#include "board_hal.h"
#include "dcd.h"

#define RCC_CRRCR_HSI48ON (1 << 0)
#define RCC_CRRCR_HSI48RDY (1 << 1)

#define USB_ENDPOINT_COUNT 8                                   // The number of USB endpoints
#define USB_ENPOINT_BUFFER_SIZE 64                             // This must be a multiple of 32
#define USB_BLSIZE (1 << 31UL)                                 // BLSIZE = 1 memory block is 32-byte large
#define USB_ADD_MASK (0x0000FFFCUL)                            // Bits 1:0 must always be written as 0b00 since packet memory is word wide and all packet buffers must be word aligned.
#define USB_TXBD_RESERVE_MASK (0b111111 << 26)                 // Bits 31:26 Reserved, must be kept at reset value.
#define USB_NUM_BLOCK(buffer_size) (((buffer_size) >> 5) - 1)  // Macro to calculate NUM_BLOCK as per Table 239. Definition of allocated buffer memory

typedef enum {
  USB_GET_STATUS = 0,         // Retrieves the status of a device, interface, or endpoint
  USB_CLEAR_FEATURE = 1,      // Clears a feature (e.g., endpoint halt)
  USB_SET_FEATURE = 3,        // Sets a feature (e.g., endpoint halt)
  USB_SET_ADDRESS = 5,        // Sets the device address after enumeration
  USB_GET_DESCRIPTOR = 6,     // Retrieves device, configuration, interface, or endpoint descriptors
  USB_SET_DESCRIPTOR = 7,     // Sets a descriptor
  USB_GET_CONFIGURATION = 8,  // Retrieves the current configuration value
  USB_SET_CONFIGURATION = 9,  // Sets the device configuration
  USB_GET_INTERFACE = 10,     // Retrieves the current interface alternate setting
  USB_SET_INTERFACE = 11,     // Sets the interface alternate setting
  USB_SYNCH_FRAME = 12        // Used for synchronization in isochronous transfers
} usb_request_code_t;

typedef enum {
  USB_REQ_TYPE_STANDARD = 0,
  USB_REQ_TYPE_CLASS = 1,
  USB_REQ_TYPE_VENDOR = 2,
  USB_REQ_TYPE_INVALID = 3
} usb_request_type_t;

// bmRequestType:
// Bit 7 (Direction): 0 for host-to-device, 1 for device-to-host.
// Bit 6-5 (Type): 00 for standard, 01 for class, 10 for vendor.
// Bit 4-0 (Recipient): 00000 for device, 00001 for interface, 00010 for endpoint, 00011 for other.

typedef struct {
  uint8_t bmRequestType;  // usb_request_type_t
  uint8_t bRequest;       // usb_request_code_t
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} __attribute__((packed)) USB_SetupRequest;

typedef struct
{
  volatile USB_DRD_PMABuffDescTypeDef EP[USB_ENDPOINT_COUNT];
} USB_DRD_PMABufferRegisters;

#define EP0_RX_BUF (0x0040 << 1)                           // EP0 rx buffer is at offset 128
#define EP0_RX_COUNT ((USB_DRD_FS->CHEP0R >> 16) & 0x3FF)  //

// USB_DRD_PMAADDR endpoint buffer registers
#define USB_PMAADDR ((volatile USB_DRD_PMABufferRegisters *)(USB_DRD_PMAADDR))

void usb_init_ep_buffers(uint32_t ep_index, uint32_t addr) {
  // Bits 31:26 Reserved, must be kept at reset value, mask rest
  uint32_t txbd = USB_PMAADDR->EP[ep_index].TXBD & USB_TXBD_RESERVE_MASK;

  // Set address
  txbd |= (addr & USB_ADD_MASK);

  // Set address value
  USB_PMAADDR->EP[ep_index].TXBD = txbd;

  // Set BLSIZE = 1 and count = USB_ENPOINT_BUFFER_SIZE / 32 all shifted
  // So for buffer size of 64 bytes then (64 / 32 - 1) = 1
  // See Table 239. Definition of allocated buffer memory
  uint32_t block_size_count = (USB_BLSIZE | (USB_NUM_BLOCK(USB_ENPOINT_BUFFER_SIZE)) << 16);

  // Set address (rx buffer is at tx buffer address + USB_ENPOINT_BUFFER_SIZE)
  uint32_t addr_masked = (addr + USB_ENPOINT_BUFFER_SIZE) & USB_ADD_MASK;

  // Set EP RX buffer address and block size and count
  USB_PMAADDR->EP[ep_index].RXBD = (block_size_count | addr_masked);
}

void usb_init_chep0_control() {
  // Clear endpoint 0 register
  USB_DRD_FS->CHEP0R = 0;

  // Configure endpoint 0
  USB_DRD_FS->CHEP0R =  // endpoint/channel address = 0
      USB_EP_CONTROL |  // EP_TYPE = CONTROL
      USB_EP_TX_NAK |   // STAT_TX = NAK - tell host not ready to transmit data yet
      USB_EP_RX_VALID;  // STAT_RX = VALID

  usb_init_ep_buffers(0, sizeof(USB_DRD_PMABufferRegisters));  // EP0 starts in USBSRAM at end of PM buffer registers
}

void usb_init_device() {
  USB_DRD_FS->CNTR = USB_CNTR_USBRST;  // Force USB reset
  USB_DRD_FS->CNTR = 0;                // Clear control register (enable bits off) - will set Device mode and relese reset
  USB_DRD_FS->ISTR = 0;                // Clear interrupt status register
  USB_DRD_FS->DADDR = 0;               // Device address = 0 (not addressed yet)

  // Initialize Endpoint 0 as control
  usb_init_chep0_control();

  // Enable USB device by setting enable bit in DADDR register
  USB_DRD_FS->DADDR = USB_DADDR_EF;

  // Enable specific USB interrupts: RESET, CTR, etc.
  USB_DRD_FS->CNTR = USB_CNTR_CTRM | USB_CNTR_RESETM | USB_CNTR_SUSPM | USB_CNTR_WKUPM | USB_CNTR_SOFM;

  // Enabling DP Pull-UP bit to Connect internal PU resistor on USB DP line
  USB_DRD_FS->BCDR |= USB_BCDR_DPPU;
}

void usb_init_hal() {
  // Set PA11 and PA12 to Alternate Function mode
  GPIO_SET_MODE(GPIOA, BIT_11_POS, MODER_ALT);
  GPIO_SET_MODE(GPIOA, BIT_12_POS, MODER_ALT);

  // Set AF0 (USB) for PA11 and PA12 (AFR = 0)
  GPIOA->AFR[1] &= ~((GPIO_AF_MSK << ((BIT_11_POS - 8) * GPIO_AF_BIT_COUNT)) | (GPIO_AF_MSK << ((BIT_12_POS - 8) * GPIO_AF_BIT_COUNT)));

  // No pull-up/pull-down
  GPIOA->PUPDR &= ~((0x11 << (BIT_11_POS * 2)) | (0x11 << (BIT_12_POS * 2)));

  // Push-pull output (default)
  GPIOA->OTYPER &= ~(BIT_11 | BIT_12);

  // Very high speed (11b)
  GPIOA->OSPEEDR &= ~((0x11 << (BIT_11_POS * 2)) | (0x11 << (BIT_12_POS * 2)));
  GPIOA->OSPEEDR |= ((0x11 << (BIT_11_POS * 2)) | (0x11 << (BIT_12_POS * 2)));

  // Set up 48MHz clock for USB
  RCC->CR |= BIT_22;                     // Enable HSI48
  while ((RCC->CR & BIT_23) == 0);       // Wait for HSI48 ready
  RCC->CCIPR2 &= ~(0b11 << BIT_12_POS);  // Select HSI48 as USB clock source

  // Enable USB peripheral clock
  RCC->APBENR1 |= RCC_APBENR1_USBEN;

  // Enable USB IRQ
  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
}

void usb_reset(void) {
  // Clear endpoint/channel registers
  for (int i = 0; i < 8; ++i) {
    (&USB_DRD_FS->CHEP0R)[i] = 0;
  }

  // Initialise ep0 (control end point)
  usb_init_chep0_control();
}

void usb_ep0_read(uint8_t *dst, uint16_t len) {
  volatile uint32_t *pma = (uint32_t *)(USB_DRD_PMAADDR + EP0_RX_BUF);
  uint16_t i = 0;

  while (i < len) {
    uint32_t w = *pma++;  // Read 32-bit word

    // Extract 4 bytes (little-endian order)
    if (i < len) dst[i++] = (w >> 0) & 0xFF;
    if (i < len) dst[i++] = (w >> 8) & 0xFF;
    if (i < len) dst[i++] = (w >> 16) & 0xFF;
    if (i < len) dst[i++] = (w >> 24) & 0xFF;
  }
}

void usb_handle_setup() {
  // Read the request from ep0 rx buffer
  USB_SetupRequest req;
  usb_ep0_read((uint8_t *)&req, sizeof(USB_SetupRequest));

  // Now decode request, e.g. GET_DESCRIPTOR, SET_ADDRESS etc.
  switch (req.bRequest) {
    case USB_GET_DESCRIPTOR:
      // Send descriptor on EP0 IN
      break;

    case USB_SET_ADDRESS:
      // Store address, send ZLP on EP0 IN
      break;

    default:
      // TODO: Stall EP0 for unsupported requests
      break;
  }
}

void USB_UCPD1_2_IRQHandler() {
  dcd_int_handler(0);  // Port 0 for most STM32 single-port USB devices
}

void USB_UCPD1_2_IRQHandler1(void) {
  uint32_t istr = USB_DRD_FS->ISTR;

  // Correct Transfer Interrupt (any endpoint)
  if (istr & USB_ISTR_CTR) {
    // Get the index of the endpoint this interrupt is for
    uint32_t ep_index = istr & USB_CHEP_ADDR;

    // Get the endpoint register
    uint32_t ep_reg = (&USB_DRD_FS->CHEP0R)[ep_index];

    // USB valid transaction received?
    if (ep_reg & USB_CHEP_VTRX) {
      // Clear RX bit
      (&USB_DRD_FS->CHEP0R)[ep_index] = ep_reg & ~USB_CHEP_VTRX;

      if (ep_index == 0) {
        usb_handle_setup();
      } else {
        // usb_handle_data_out(ep_index);
      }
    }

    // Valid USB transaction transmitted?
    if (ep_reg & USB_CHEP_VTTX) {
      // Clear TX bit
      (&USB_DRD_FS->CHEP0R)[ep_index] = ep_reg & ~USB_CHEP_VTTX;

      if (ep_index == 0) {
        // usb_handle_ep0_tx();
      } else {
        // usb_handle_data_in(ep_index);
      }
    }
  }

  // Reset Interrupt — reinitialize device
  if (istr & USB_ISTR_RESET) {
    USB_DRD_FS->ISTR &= ~USB_ISTR_RESET;

    // Reset USB to known state
    usb_reset();
  }

  // Suspend Interrupt — usually enter low power
  if (istr & USB_ISTR_SUSP) {
    USB_DRD_FS->ISTR &= ~USB_ISTR_SUSP;
    // usb_suspend();  // user-defined: reduce clocks, save state, etc.
  }

  // Wakeup Interrupt — exit low power
  if (istr & USB_ISTR_WKUP) {
    USB_DRD_FS->ISTR &= ~USB_ISTR_WKUP;
    // usb_resume();  // user-defined: restore clocks, state
  }

  // Start of Frame Interrupt — 1ms SOF
  if (istr & USB_ISTR_SOF) {
    USB_DRD_FS->ISTR &= ~USB_ISTR_SOF;
    // usb_sof();  // optional: for timing sync or isochronous transfers
  }
}

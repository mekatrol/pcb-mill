
#ifndef TUSB_FSDEV_STM32_H
#define TUSB_FSDEV_STM32_H

#include "stm32g0xx.h"
#define FSDEV_PMA_SIZE (2048u)

#define USB_EP_CTR_RX USB_EP_VTRX
#define USB_EP_CTR_TX USB_EP_VTTX
#define USB_EP_T_FIELD USB_CHEP_UTYPE
#define USB_EPREG_MASK USB_CHEP_REG_MASK
#define USB_EPTX_DTOGMASK USB_CHEP_TX_DTOGMASK
#define USB_EPRX_DTOGMASK USB_CHEP_RX_DTOGMASK
#define USB_EPTX_DTOG1 USB_CHEP_TX_DTOG1
#define USB_EPTX_DTOG2 USB_CHEP_TX_DTOG2
#define USB_EPRX_DTOG1 USB_CHEP_RX_DTOG1
#define USB_EPRX_DTOG2 USB_CHEP_RX_DTOG2
#define USB_EPRX_STAT USB_CH_RX_VALID
#define USB_EPKIND_MASK USB_EP_KIND_MASK
#define USB_CNTR_FRES USB_CNTR_USBRST
#define USB_CNTR_RESUME USB_CNTR_L2RES
#define USB_ISTR_EP_ID USB_ISTR_IDN
#define USB_EPADDR_FIELD USB_CHEP_ADDR
#define USB_CNTR_LPMODE USB_CNTR_SUSPRDY
#define USB_CNTR_FSUSP USB_CNTR_SUSPEN

//--------------------------------------------------------------------+
// Register and PMA Base Address
//--------------------------------------------------------------------+
#define FSDEV_PMA_BASE USB_DRD_PMAADDR

#define USB_ISTR_L1REQ_FORCED (USB_ISTR_L1REQ)
#define USB_ISTR_ALL_EVENTS (USB_ISTR_PMAOVR | USB_ISTR_ERR | USB_ISTR_WKUP | USB_ISTR_SUSP | \
                             USB_ISTR_RESET | USB_ISTR_SOF | USB_ISTR_ESOF | USB_ISTR_L1REQ_FORCED)

void dcd_int_enable() {
  // forces write to RAM before allowing ISR to execute
  __DSB();
  __ISB();

  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
}

void dcd_int_disable() {
  NVIC_DisableIRQ(USB_UCPD1_2_IRQn);
}

void dcd_disconnect() {
  USB->BCDR &= ~(USB_BCDR_DPPU);
}

#endif /* TUSB_FSDEV_STM32_H */

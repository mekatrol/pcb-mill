 # USB initialisation and configuration sequence
 
## USB Initialization Sequence (Device Side)
✅ Step 1: Enable and Configure USB Peripheral
Enable USB clock (e.g. HSI48 source and RCC setup)

Configure USB data pins (PA11/PA12) in AF14

Enable USB device by writing to USB->DADDR

✅ Step 2: Wait for USB RESET
When connected to a PC, the host pulls D+ low briefly, triggering a USB reset

Device detects this via USB->ISTR & USB_ISTR_RESET (bit 10)

In response:
> Clear all endpoint registers  
> Set the BTABLE (USB buffer table) to a known location (typically 0x00)  
> Prepare endpoint 0 for control transfers (SETUP packets)  

✅ Step 3: Handle Host Enumeration
The host starts sending requests to identify and configure the device.

This happens over Endpoint 0 (EP0) and includes:

Request (host → device)	Purpose
GET_DESCRIPTOR	Asks for device, configuration, string descriptors
SET_ADDRESS	Host assigns a USB address
SET_CONFIGURATION	Host enables communication

## 📦 USB CDC Descriptors (Device Identity)
The device must respond with a hierarchy of descriptors:

Device Descriptor (tells host it's a CDC device)

Configuration Descriptor

Interface Descriptors:

Communication Class Interface (for control/notifications)

Data Class Interface (for TX/RX data)

Endpoint Descriptors:

EP0: control

EP1 IN: interrupt (optional status notifications)

EP3 OUT: bulk OUT (PC to device)

EP3 IN: bulk IN (device to PC)

The host reads these via GET_DESCRIPTOR requests.

🔁 3. SETUP Packet Handling (EP0)
All control messages come in the form of SETUP packets (8 bytes):

c
Copy
Edit
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} USB_SetupPacket;
Your code needs to:

Parse these 8 bytes from EP0 buffer

Respond with:

a data stage (if requested),

and then a status stage

For CDC devices, these requests include both standard (e.g. SET_ADDRESS) and class-specific (e.g. SET_LINE_CODING) types.

## 💬 Virtual COM Port Communication
After enumeration:

Host sets configuration (SET_CONFIGURATION)

Host opens a COM port (e.g. COM5, /dev/ttyUSB0)

Windows/Linux driver sends:

SET_LINE_CODING (9600 baud, 8N1, etc.)

SET_CONTROL_LINE_STATE (DTR/RTS flags)

Then:

Bulk IN (EP3 IN): used to send data from device to PC

Bulk OUT (EP3 OUT): used to receive data from PC

You handle these transfers by reading from/writing to PMA memory and setting endpoint status flags.

## ⚙️ Interrupts You Must Handle
ISR Flag	Meaning
USB_ISTR_RESET	Reinitialize device on reset
USB_ISTR_CTR	Correct transfer on endpoint
USB_ISTR_SUSP	USB suspend mode (low power)

In the USB_UCPD1_2_IRQHandler, check these flags and act accordingly.

🔄 Example Data Flow
PC opens COM port

PC sends SET_LINE_CODING (baud/stop bits/parity)

PC sends SET_CONTROL_LINE_STATE (RTS/DTR)

PC writes data → EP3 OUT → your firmware receives

Your firmware sends reply → EP3 IN → PC reads data

⚠️ Notes
All USB communication is host-driven — your device only responds or prepares data

USB uses token-based protocol, with setup/data/status stages

CDC devices are supported natively in most OSes (no special driver needed)
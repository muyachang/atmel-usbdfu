/* \file
 *
 * Header file for Descriptors.c.
 */

#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

#include <LUFA/Drivers/USB/USB.h>

#define ATTR_WILL_DETATCH           _BV(3) // the DFU device will detach and re-attach when a DFU_DETACH command is issued, rather than the host issuing a USB Reset. */
#define ATTR_MANEFESTATION_TOLERANT _BV(2) // the DFU device can communicate during the manifestation phase (memory programming phase). */
#define ATTR_CAN_UPLOAD             _BV(1) // the DFU device can accept DFU_UPLOAD requests to send data from the device to the host. */    
#define ATTR_CAN_DOWNLOAD           _BV(0) // the DFU device can accept DFU_DNLOAD requests to send data from the host to the device. */    

#define VENDOR_ID_CODE  0x03EB // Atmel
#define PRODUCT_ID_CODE 0x2FF0 // ATmega32U2

typedef struct
{
  uint8_t  bLength;            // Size of this descriptor, in bytes.
  uint8_t  bDescriptorType;    // DEVICE descriptor type.
  uint16_t bcdUSB;             // USB specification release number in binary coded decimal.
  uint8_t  bDeviceClass;       // See interface.
  uint8_t  bDeviceSubClass;    // See interface.
  uint8_t  bDeviceProtocol;    // See interface.
  uint8_t  bMaxPacketSize0;    // Maximum packet size for endpoint zero.
  uint16_t idVendor;           // Vendor ID. Assigned by the USB-IF.
  uint16_t idProduct;          // Product ID. Assigned by manufacturer
  uint16_t bcdDevice;          // Device release number in binary coded decimal.
  uint8_t  iManufacturer;      // Index of string descriptor.
  uint8_t  iProduct;           // Index of string descriptor.
  uint8_t  iSerialNumber;      // Index of string descriptor.
  uint8_t  bNumConfigurations; // One configuration only for DFU.
} USB_DFU_Device_Descriptor_t;

typedef struct
{
  uint8_t  bLength;             // Size of this descriptor, in bytes.
  uint8_t  bDescriptorType;     // CONFIGURATION descriptor type.
  uint16_t wTotalLength;        // Total length of data returned for this configuration. Includes the combined length 
                                // of all descriptors (configuration, interface, endpoint, and class or vendor specific)
                                // returned for this configuration.
  uint8_t  bNumInterfaces;      // Number of interfaces supported by this configuration
  uint8_t  bConfigurationValue; // Value to use as an argument to Set Configuration to select this configuration
  uint8_t  iConfiguration;      //  Index of string descriptor describing this configuration
  uint8_t  bmAttributes;        // Configuration characteristics
  uint8_t  MaxPower;            // Maximum power consumption of USB device from the bus in this specific configuration
                                // when the device is fully operational.  Expressed in 2 mA units (i.e., 50 = 100 mA).
} USB_DFU_Configuration_Descriptor_t;

typedef struct
{
  uint8_t  bLength;            // Size of this descriptor, in bytes.
  uint8_t  bDescriptorType;    // INTERFACE descriptor type.
  uint8_t  bInterfaceNumber;   // Number of this interface.
  uint8_t  bAlternateSetting;  // Alternate setting. *
  uint8_t  bNumEndpoints;      // Only the control pipe is used.
  uint8_t  bInterfaceClass;    // Application Specific Class Code
  uint8_t  bInterfaceSubClass; // Device Firmware Upgrade Code
  uint8_t  bInterfaceProtocol; // DFU mode protocol.
  uint8_t  iInterface;         // Index of string descriptor for this interface.
} USB_DFU_Interface_Descriptor_t;

typedef struct
{
  uint8_t  bLength;         // Size of this descriptor, in bytes.
  uint8_t  bDescriptorType; // DFU FUNCTIONAL descriptor type.
  uint8_t  bmAttributes;    // DFU attributes
  uint16_t wDetachTimeOut;  // Time, in milliseconds, that the device will wait after receipt of the DFU_DETACH request.
  uint16_t wTransferSize;   // Maximum number of bytes that the device can accept per control-write transaction.
  uint16_t bcdDFUVersion;   // Numeric expression identifying the version of the DFU Specification release.
} USB_DFU_Functional_Descriptor_t;

typedef struct
{
  USB_DFU_Device_Descriptor_t        Device;
  USB_DFU_Configuration_Descriptor_t Config;
  USB_DFU_Interface_Descriptor_t     Interface;
  USB_DFU_Functional_Descriptor_t    Functional;
} DFU_Mode_Descriptor_Set_t;

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue, const uint8_t wIndex, const void** const DescriptorAddress) ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

#endif

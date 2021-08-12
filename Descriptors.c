
/* \file
 *
 * USB Device Descriptors, for library use when in USB device mode. Descriptors are special 
 * computer-readable structures which the host requests upon device enumeration, to determine
 * the device's capabilities and functions.  
 */

#include "Descriptors.h"

const DFU_Mode_Descriptor_Set_t DFU_Mode_Descriptor_Set =
{
  .Device = 
    {
      .bLength             = sizeof(USB_DFU_Device_Descriptor_t), // Should be 0x12
      .bDescriptorType     = 0x01,
      .bcdUSB              = 0x0100,
      .bDeviceClass        = 0x00,
      .bDeviceSubClass     = 0x00,
      .bDeviceProtocol     = 0x00,
      .bMaxPacketSize0     = FIXED_CONTROL_ENDPOINT_SIZE,
      .idVendor            = VENDOR_ID_CODE,
      .idProduct           = PRODUCT_ID_CODE,
      .bcdDevice           = 0x0000,
      .iManufacturer       = 0x00,
      .iProduct            = 0x00,
      .iSerialNumber       = 0x00,
      .bNumConfigurations  = 0x01 
    },

  .Config = 
    {
      .bLength             = sizeof(USB_DFU_Configuration_Descriptor_t), // Should be 0x09
      .bDescriptorType     = 0x02,
      .wTotalLength        = sizeof(USB_DFU_Configuration_Descriptor_t) +
                             sizeof(USB_DFU_Interface_Descriptor_t) + 
                             sizeof(USB_DFU_Functional_Descriptor_t),
                             // should be 0x1B
      .bNumInterfaces      = 1,
      .bConfigurationValue = 1,
      .iConfiguration      = 0x00,
      .bmAttributes        = USB_CONFIG_ATTR_BUSPOWERED,
      .MaxPower            = USB_CONFIG_POWER_MA(100)
    },
    
  .Interface = 
    {
      .bLength             = sizeof(USB_DFU_Interface_Descriptor_t), // should be 0x09
      .bDescriptorType     = 0x04,
      .bInterfaceNumber    = 0x00,
      .bAlternateSetting   = 0x00,
      .bNumEndpoints       = 0x00,
      .bInterfaceClass     = 0xFE,
      .bInterfaceSubClass  = 0x01,
      .bInterfaceProtocol  = 0x00,
      .iInterface          = 0x00
    },
    
  .Functional = 
    {
      .bLength             = sizeof(USB_DFU_Functional_Descriptor_t), // should be 0x09
      .bDescriptorType     = 0x21,
      .bmAttributes        = (ATTR_MANEFESTATION_TOLERANT | ATTR_CAN_UPLOAD | ATTR_CAN_DOWNLOAD),
      .wDetachTimeOut      = 0,
      .wTransferSize       = 3072,
      .bcdDFUVersion       = 0x0101
    }

};

const USB_Descriptor_String_t LanguageString =
{
  .Header         = {.Size = USB_STRING_LEN(1), .Type = DTYPE_String},
  .UnicodeString  = {LANGUAGE_ID_ENG}
};

const USB_Descriptor_String_t ManufacturerString =
{
  .Header        = {.Size = USB_STRING_LEN(5), .Type = DTYPE_String},
  .UnicodeString = L"ICSRL"
};

const USB_Descriptor_String_t ProductString =
{
  .Header         = {.Size = USB_STRING_LEN(19), .Type = DTYPE_String},
  .UnicodeString  = L"ICSRL RRAM Testchip"
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue, const uint8_t wIndex, const void** const DescriptorAddress)
{
  const uint8_t DescriptorType   = (wValue >> 8);
  const uint8_t DescriptorNumber = (wValue & 0xFF);

  void*    Address = NULL;
  uint16_t Size    = NO_DESCRIPTOR;

  switch (DescriptorType)
  {
    case DTYPE_Device:
      Address = (void*)&(DFU_Mode_Descriptor_Set.Device);
      Size    = DFU_Mode_Descriptor_Set.Device.bLength;
      break;
    case DTYPE_Configuration: 
      Address = (void*)&(DFU_Mode_Descriptor_Set.Config);
      Size    = DFU_Mode_Descriptor_Set.Config.wTotalLength;
      break;
    case DTYPE_String: 
      switch (DescriptorNumber)
      {
        case 0x00: 
          Address = (void*)&(LanguageString);
					Size    = LanguageString.Header.Size;
          break;
        case 0x01: 
          Address = (void*)&(ManufacturerString);
					Size    = ManufacturerString.Header.Size;
          break;
        case 0x02: 
          Address = (void*)&(ProductString);
					Size    = ProductString.Header.Size;
          break;
      }
      break;
  }
  
  *DescriptorAddress = Address;
  return Size;
}

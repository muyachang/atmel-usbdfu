
/** \file
 *
 *  Header file for rram-usbdfu.c.
 */

#ifndef _RRAM_USB_DFU_BOOTLOADER_H_
#define _RRAM_USB_DFU_BOOTLOADER_H_

#include <avr/wdt.h>

#include <LUFA/Drivers/Board/Dataflash.h>

#include "Descriptors.h"

/** Bootloader Information */
#define BOOTLOADER_VERSION_MAJOR 2
#define BOOTLOADER_VERSION_MINOR 0
#define BOOTLOADER_VERSION       ((BOOTLOADER_VERSION_MAJOR << 4) | BOOTLOADER_VERSION_MINOR)
#define BOOTLOADER_ID_BYTE1      0xDC
#define BOOTLOADER_ID_BYTE2      0xFB

/** Device Information */
#define MANUFACTURER_CODE 0x1E
#define FAMILY_CODE       0x94
#define PRODUCT_NAME      0x13
#define PRODUCT_REVISION  0x14

/** DFU class command requests */
enum DFU_Class_Specific_Request_t
{
  DFU_DETACH    = 0, 
  DFU_DNLOAD    = 1,
  DFU_UPLOAD    = 2,
  DFU_GETSTATUS = 3,
  DFU_CLRSTATUS = 4,
  DFU_GETSTATE  = 5,
  DFU_ABORT     = 6
};

enum DFU_Status_t
{
  OK              = 0,
  errTARGET       = 1,
  errFILE         = 2,
  errWRITE        = 3,
  errERASE        = 4,
  errCHECK_ERASED = 5,
  errPROG         = 6,
  errVERIFY       = 7,
  errADDRESS      = 8,
  errNOTDONE      = 9,
  errFIRMWARE     = 10,
  errVENDOR       = 11,
  errUSBR         = 12,
  errPOR          = 13,
  errUNKNOWN      = 14,
  errSTALLEDPKT   = 15
};
    
enum DFU_State_t
{
  appIDLE                = 0,
  appDETACH              = 1,
  dfuIDLE                = 2,
  dfuDNLOAD_SYNC         = 3,
  dfuDNBUSY              = 4,
  dfuDNLOAD_IDLE         = 5,
  dfuMANIFEST_SYNC       = 6,
  dfuMANIFEST            = 7,
  dfuMANIFEST_WAIT_RESET = 8,
  dfuUPLOAD_IDLE         = 9,
  dfuERROR               = 10
};

/** Flip commands */
typedef struct
{
  uint8_t group;
  uint8_t data[5];
} USB_FLIP_Command_t;

enum FLIP_Group_Command_t
{
  CMD_GROUP_DOWNLOAD = 1, 
  CMD_GROUP_UPLOAD   = 3,
  CMD_GROUP_EXEC     = 4,
  CMD_GROUP_READ     = 5,
  CMD_GROUP_SELECT   = 6
};

/** Type define for a non-returning function pointer to the loaded application. */
typedef void (*AppPtr_t)(void) ATTR_NO_RETURN;

void SetupHardware(void);
void ResetHardware(void);
void DiscardFillerBytes(uint8_t NumberOfBytes);
void ProcessFlipCommand(void);

void ProcessDownload(void);
void ProcessUpload(void);
void ProcessExec(void);
void ProcessRead(void);
void ProcessSelect(void);
    
void UpdateState(void);
void EVENT_USB_Device_UnhandledControlRequest(void);

#endif /* _RRAM_USB_DFU_BOOTLOADER_H_ */

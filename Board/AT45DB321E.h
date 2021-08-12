
/** \file
 *  \brief Board specific Dataflash commands header for the AT45DB321E.
 *
 *  Board specific Dataflash commands header for the AT45DB321E.
 *
 *  \note This file should not be included directly. It is automatically included as needed by the dataflash driver
 *        dispatch header located in LUFA/Drivers/Board/Dataflash.h.
 */

/** \ingroup Group_Dataflash_RRAM_TESTCHIP
 *  @defgroup Group_Dataflash_RRAM_TESTCHIP_AT45DB321E AT45DB321E
 *
 *  Board specific Dataflash commands header for the AT45DB321E.
 *
 *  \note This file should not be included directly. It is automatically included as needed by the dataflash driver
 *        dispatch header located in LUFA/Drivers/Board/Dataflash.h.
 *
 *  @{
 */

#ifndef __DATAFLASH_CMDS_H__
#define __DATAFLASH_CMDS_H__

  /* Status Register Format */
  #define DF_STATUSREG_BYTE1_READY                         (1 << 7)
  #define DF_STATUSREG_BYTE1_COMPMISMATCH                  (1 << 6)
  #define DF_STATUSREG_BYTE1_SECTORPROTECTION_ON           (1 << 1)
  #define DF_STATUSREG_BYTE1_PAGESIZE                      (1 << 0)
  #define DF_STATUSREG_BYTE2_READY                         (1 << 7)
  #define DF_STATUSREG_BYTE2_ERASEPROGRAMERRORDETECTED     (1 << 5)
  #define DF_STATUSREG_BYTE2_SECTORLOCKDOWN_ON             (1 << 3)
  #define DF_STATUSREG_BYTE2_SECTORPROGRAMSUSPENDED_BUFF2  (1 << 2)
  #define DF_STATUSREG_BYTE2_SECTORPROGRAMSUSPENDED_BUFF1  (1 << 1)
  #define DF_STATUSREG_BYTE2_SECTORERASESUSPENDED          (1 << 0)

  /* Manufacturer and Device ID Information */
  #define DF_MANUFACTURER_ID                      0x1F // Manufacturer ID
  #define DF_DEVICE_ID_BYTE1                      0x27 // Device ID (Byte 1)
  #define DF_DEVICE_ID_BYTE2                      0x01 // Device ID (Byte 2)
  #define DF_EDI_STRING_LEN                       0x01 // Extended Device Information (EDI) String Length
  #define DF_EDI_BYTE1                            0x01 // [Optional to Read] EDI Byte 1

  /* Read Commands */
  #define DF_CMD_MAINMEMPAGEREAD                  0xD2 // Main Memory Page Read
  #define DF_CMD_CONTARRAYREAD_LP                 0x01 // Continuous Array Read (Low Power Mode) - 15MHz
  #define DF_CMD_CONTARRAYREAD_LF                 0x03 // Continuous Array Read (Low Frequency)  - 50MHz
  #define DF_CMD_CONTARRAYREAD_HF                 0x0B // Continuous Array Read (High Frequency) - 85MHz
  #define DF_CMD_CONTARRAYREAD_HHF                0x1B // Continuous Array Read (High Frequency) - 104MHz
  #define DF_CMD_BUFF1READ_LF                     0xD1 // Buffer 1 Read (Low Frequency)  - 50MHz
  #define DF_CMD_BUFF2READ_LF                     0xD3 // Buffer 2 Read (Low Frequency)  - 50MHz
  #define DF_CMD_BUFF1READ_HF                     0xD4 // Buffer 1 Read (High Frequency) - 85MHz
  #define DF_CMD_BUFF2READ_HF                     0xD6 // Buffer 2 Read (High Frequency) - 85MHz

  /* Program and Erase Commands */
  #define DF_CMD_BUFF1WRITE                       0x84 // Buffer 1 Write
  #define DF_CMD_BUFF2WRITE                       0x87 // Buffer 2 Write
  #define DF_CMD_BUFF1TOMAINMEMWITHERASE          0x83 // Buffer 1 to Main Memory Page Program with Built-In Erase
  #define DF_CMD_BUFF2TOMAINMEMWITHERASE          0x86 // Buffer 2 to Main Memory Page Program with Built-In Erase
  #define DF_CMD_BUFF1TOMAINMEM                   0x88 // Buffer 1 to Main Memory Page Program without Built-In Erase
  #define DF_CMD_BUFF2TOMAINMEM                   0x89 // Buffer 2 to Main Memory Page Program without Built-In Erase
  #define DF_CMD_MAINMEMPAGETHROUGHBUFF1WITHERASE 0x82 // Main Memory Page Program through Buffer 1 with Built-In Erase
  #define DF_CMD_MAINMEMPAGETHROUGHBUFF2WITHERASE 0x85 // Main Memory Page Program through Buffer 2 with Built-In Erase
  #define DF_CMD_MAINMEMPAGETHROUGHBUFF1          0x82 // Main Memory Page Program through Buffer 1 without Built-In Erase
  #define DF_CMD_PAGEERASE                        0x81 // Page Erase
  #define DF_CMD_BLOCKERASE                       0x50 // Block Erase
  #define DF_CMD_SECTORERASE                      0x7C // Sector Erase
  #define DF_CMD_CHIPERASE                        ((char[]){0xC7, 0x94, 0x80, 0x9A}) // Chip Erase
  #define DF_CMD_PROGRAMERASESUSPEND              0xB0 // Program/Erase Suspend
  #define DF_CMD_PROGRAMERASERESUME               0xD0 // Program/Erase Resume
  #define DF_CMD_RMWTHROUGHBUFF1                  0x58 // Read-Modify-Write through Buffer 1
  #define DF_CMD_RMWTHROUGHBUFF2                  0x59 // Read-Modify-Write through Buffer 2

  /* Protection and Security Commands */
  #define DF_CMD_ENABLESECTORPROTECTION           ((char[]){0x3D, 0x2A, 0x7F, 0xA9})
  #define DF_CMD_DISABLESECTORPROTECTION          ((char[]){0x3D, 0x2A, 0x7F, 0x9A})
  #define DF_CMD_ERASESECTORPROTECTIONREG         ((char[]){0x3D, 0x2A, 0x7F, 0xCF})
  #define DF_CMD_PROGRAMSECTORPROTECTIONREG       ((char[]){0x3D, 0x2A, 0x7F, 0xFC})
  #define DF_CMD_READSECTORPROTECTIONREG          0x32
  #define DF_CMD_SECTORLOCKDOWN                   ((char[]){0x3D, 0x2A, 0x7F, 0x30})
  #define DF_CMD_READSECTORLOCKDOWNREG            0x35
  #define DF_CMD_FREEZESECTORLOCKDOWN             ((char[]){0x34, 0x55, 0xAA, 0x40})
  #define DF_CMD_PROGRAMSECURITYREG               ((char[]){0x9B, 0x00, 0x00, 0x00})
  #define DF_CMD_READSECURITYREG                  0x77

  /* Additional Commands */
  #define DF_CMD_MAINMEMTOBUFF1                   0x53 // Main Memory Page to Buffer 1 Transfer
  #define DF_CMD_MAINMEMTOBUFF2                   0x55 // Main Memory Page to Buffer 2 Transfer
  #define DF_CMD_MAINMEMTOBUFF1COMP               0x60 // Main Memory Page to Buffer 1 Compare
  #define DF_CMD_MAINMEMTOBUFF2COMP               0x61 // Main Memory Page to Buffer 2 Compare
  #define DF_CMD_AUTOREWRITEBUFF1                 0x58 // Auto Page Rewrite through Buffer 1
  #define DF_CMD_AUTOREWRITEBUFF2                 0x59 // Auto Page Rewrite through Buffer 2
  #define DF_CMD_DEEPPOWERDOWN                    0xB9 // Deep Power-Down
  #define DF_CMD_RESUMEFROMDEEPPOWERDOWN          0xAB // Resume from Deep Power-Down
  #define DF_CMD_ULTRADEEPPOWERDOWN               0x79 // Ultra-Deep Power-Down
  #define DF_CMD_GETSTATUS                        0xD7 // Status Register Read
  #define DF_CMD_READMANUFACTURERDEVICEINFO       0x9F // Manufacturer and Device ID Read
  #define DF_CMD_CONFPWR2PAGESIZE                 ((char[]){0x3D, 0x2A, 0x80, 0xA6}) // Configure “Power of 2” (Binary) Page Size
  #define DF_CMD_CONFSTDPAGESIZE                  ((char[]){0x3D, 0x2A, 0x80, 0xA7}) // Configure Standard DataFlash Page Size
  #define DF_CMD_SOFTWARERESET                    ((char[]){0xF0, 0x00, 0x00, 0x00}) // Software Reset

#endif

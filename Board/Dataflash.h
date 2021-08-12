/*
   Board Dataflash driver for the RRAM Testchip
*/

#ifndef __DATAFLASH_RRAM_TESTCHIP_H__
#define __DATAFLASH_RRAM_TESTCHIP_H__

  /* Includes: */
    #include "AT45DB321E.h"

  /* Preprocessor Checks: */
    #if !defined(__INCLUDE_FROM_DATAFLASH_H)
      #error Do not include this file directly. Include LUFA/Drivers/Board/Dataflash.h instead.
    #endif

  /* Private Interface - For use in library only: */
  #if !defined(__DOXYGEN__)
    /* Macros: */
      #define DATAFLASH_CHIPCS_PORT  PORTB
      #define DATAFLASH_CHIPCS_DDR   DDRB
      #define DATAFLASH_CHIPCS_MASK  (1<<4)
  #endif

  /* Public Interface - May be used in end-application: */
    /* Macros: */
      #define DATAFLASH_TOTALCHIPS        1
      #define DATAFLASH_NO_CHIP           DATAFLASH_CHIPCS_MASK
      #define DATAFLASH_CHIP1             (DATAFLASH_CHIPCS_MASK ^ (1<<4))
      #define DATAFLASH_CHIP2             // TODO: Replace with mask to hold /CS of second Dataflash low, and all others high
      #define DATAFLASH_PAGE_SIZE         512
      #define DATAFLASH_PAGES             8192
      #define DATAFLASH_PAGE_ADDR_WIDTH   13
      #define DATAFLASH_OFFSET_ADDR_WIDTH  9 

    /* Inline Functions: */
      /*
       * Initialises the dataflash driver so that commands and data may be sent to an attached dataflash IC.
       *  The AVR's SPI driver MUST be initialized before any of the dataflash commands are used.
       */
      static inline void Dataflash_Init(void)
      {
        DATAFLASH_CHIPCS_PORT |=  DATAFLASH_CHIPCS_MASK; // Enable the pull up resistors
        DATAFLASH_CHIPCS_DDR  &= ~DATAFLASH_CHIPCS_MASK; // Set it as input 
      }

      /*
       * Shutdown the dataflash
       */
      static inline void Dataflash_ShutDown(void)
      {
        DATAFLASH_CHIPCS_PORT &= ~DATAFLASH_CHIPCS_MASK;
      }

      /*
       * Sends a set of page and buffer address bytes to the currently selected dataflash IC, for use with
       *  dataflash commands which require a complete 24-byte address.
       *
       *  \param[in] PageAddress  Page address within the selected dataflash IC
       *  \param[in] BufferByte   Address within the dataflash's buffer
       */
      static inline void Dataflash_SendAddressBytes(uint16_t PageAddress, const uint16_t BufferByte)
      {
      }

      /** Determines the currently selected dataflash chip.
       *
       *  \return Mask of the currently selected Dataflash chip, either \ref DATAFLASH_NO_CHIP if no chip is selected
       *          or a DATAFLASH_CHIPn mask (where n is the chip number).
       */
      static inline uint8_t Dataflash_GetSelectedChip(void) ATTR_ALWAYS_INLINE ATTR_WARN_UNUSED_RESULT;
      static inline uint8_t Dataflash_GetSelectedChip(void)
      {
        return (DATAFLASH_CHIPCS_PORT & DATAFLASH_CHIPCS_MASK); // Active low
      }

      /** Selects the given dataflash chip.
       *
       *  \param[in]  ChipMask  Mask of the Dataflash IC to select, in the form of DATAFLASH_CHIPn mask (where n is
       *              the chip number).
       */
      static inline void Dataflash_SelectChip(const uint8_t ChipMask) ATTR_ALWAYS_INLINE;
      static inline void Dataflash_SelectChip(const uint8_t ChipMask)
      {
        DATAFLASH_CHIPCS_DDR  |= DATAFLASH_CHIPCS_MASK; // Set it as output
        DATAFLASH_CHIPCS_PORT  = (DATAFLASH_CHIPCS_PORT & ~DATAFLASH_CHIPCS_MASK) | ChipMask;
      }

      /** Deselects the current dataflash chip, so that no dataflash is selected. */
      static inline void Dataflash_DeselectChip(void) ATTR_ALWAYS_INLINE;
      static inline void Dataflash_DeselectChip(void)
      {
        DATAFLASH_CHIPCS_PORT  = (DATAFLASH_CHIPCS_PORT & ~DATAFLASH_CHIPCS_MASK) | DATAFLASH_NO_CHIP;
        DATAFLASH_CHIPCS_DDR  &= ~DATAFLASH_CHIPCS_MASK; // Set it as input 
      }

      /** Selects a dataflash IC from the given page number, which should range from 0 to
       *  ((DATAFLASH_PAGES * DATAFLASH_TOTALCHIPS) - 1). For boards containing only one
       *  dataflash IC, this will select DATAFLASH_CHIP1. If the given page number is outside
       *  the total number of pages contained in the boards dataflash ICs, all dataflash ICs
       *  are deselected.
       *
       *  \param[in] PageAddress  Address of the page to manipulate, ranging from
       *                          ((DATAFLASH_PAGES * DATAFLASH_TOTALCHIPS) - 1).
       */
      static inline void Dataflash_SelectChipFromPage(const uint16_t PageAddress)
      {
        Dataflash_DeselectChip();

        if (PageAddress >= DATAFLASH_PAGES)
          return;

        #if (DATAFLASH_TOTALCHIPS == 2)
          if (PageAddress & 0x01)
            Dataflash_SelectChip(DATAFLASH_CHIP2);
          else
            Dataflash_SelectChip(DATAFLASH_CHIP1);
        #else
          Dataflash_SelectChip(DATAFLASH_CHIP1);
        #endif
      }

      /** Toggles the select line of the currently selected dataflash IC, so that it is ready to receive
       *  a new command.
       */
      static inline void Dataflash_ToggleSelectedChipCS(void)
      {
        uint8_t SelectedChipMask = Dataflash_GetSelectedChip();
        Dataflash_DeselectChip();
        Dataflash_SelectChip(SelectedChipMask);
      }

      /** Spin-loops while the currently selected dataflash is busy executing a command, such as a main
       *  memory page program or main memory to buffer transfer.
       */
      static inline void Dataflash_WaitWhileBusy(void)
      {
        Dataflash_SendByte(DF_CMD_GETSTATUS);
        while (!(Dataflash_ReceiveByte() & DF_STATUSREG_BYTE1_READY));
        Dataflash_ToggleSelectedChipCS();
      }

      /** 
       *  
       */
      static inline void Dataflash_Configure_Read_Address(uint8_t _read_command, uint32_t _addr){

        // Read command
        Dataflash_SendByte(_read_command);

        // Address
        Dataflash_SendByte((_addr >> 16) & 0xFF);
        Dataflash_SendByte((_addr >>  8) & 0xFF);
        Dataflash_SendByte((_addr >>  0) & 0xFF);

        // Dummy bytes 
        uint8_t dummy_cycles = 0;
        switch(_read_command){
          //case DF_CMD_MAINMEMPAGEREAD   : dummy_cycles = 4; break;
          case DF_CMD_CONTARRAYREAD_LP  : dummy_cycles = 0; break;
          case DF_CMD_CONTARRAYREAD_LF  : dummy_cycles = 0; break;
          //case DF_CMD_CONTARRAYREAD_HF  : dummy_cycles = 1; break;
          //case DF_CMD_CONTARRAYREAD_HHF : dummy_cycles = 2; break;
          //case DF_CMD_BUFF1READ_LF      : dummy_cycles = 1; break;
          //case DF_CMD_BUFF2READ_LF      : dummy_cycles = 1; break;
          //case DF_CMD_BUFF1READ_HF      : dummy_cycles = 1; break;
          //case DF_CMD_BUFF2READ_HF      : dummy_cycles = 1; break;
        }

        for(uint8_t i=0;i<dummy_cycles;i++)
          Dataflash_SendByte(0);

      }

      /** 
       *  
       */
      static inline void Dataflash_Configure_Read_Page_Offset(uint8_t _read_command, uint16_t _page, uint16_t _offset){
        Dataflash_Configure_Read_Address(_read_command, ((uint32_t)_page << DATAFLASH_OFFSET_ADDR_WIDTH) + (uint32_t)_offset);
      }

      /** 
       *  
       */
      static inline void Dataflash_Configure_Write_Address(uint8_t _write_command, uint32_t _addr){

        // Write command
        Dataflash_SendByte(_write_command);

        // Address
        Dataflash_SendByte((_addr >> 16) & 0xFF);
        Dataflash_SendByte((_addr >>  8) & 0xFF);
        Dataflash_SendByte((_addr >>  0) & 0xFF);

      }

      /** 
       *  
       */
      static inline void Dataflash_Configure_Write_Page_Offset(uint8_t _write_command, uint16_t _page, uint16_t _offset){
        Dataflash_Configure_Write_Address(_write_command, ((uint32_t)_page << DATAFLASH_OFFSET_ADDR_WIDTH) + (uint32_t)_offset);
      }

#endif


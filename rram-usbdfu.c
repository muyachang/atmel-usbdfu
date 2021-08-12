
/* \file
 *
 * Main source file for the DFU class bootloader. This file contains the complete bootloader logic.
 */

#include "rram-usbdfu.h"

/** Flag to indicate if the bootloader should be running, or should exit and allow the application code to run
 *  via a soft reset. When cleared, the bootloader will abort, the USB interface will shut down and the application
 *  jumped to via an indirect jump to location 0x0000 (or other location specified by the host).
 */
bool waitForSecondRequest;
USB_FLIP_Command_t flipCommand;
uint8_t DFU_State = dfuIDLE;
uint8_t DFU_Status = OK;
uint16_t nonBlankAddr;

/** Pointer to the start of the user application. By default this is 0x0000 (the reset vector), however the host
 *  may specify an alternate address when issuing the application soft-start command.
 */
AppPtr_t AppStartPtr = (AppPtr_t)0x0000;

/** A 64KB page contains 0x10000 values (0 to 0xFFFF).
 *  This is concatenated with the current 16-bit address on USB AVRs containing more than 64KB of flash memory.
 */
uint8_t curFlash64KBPageNumber = 0;

/** Main program entry point. This routine configures the hardware required by the bootloader, then continuously 
 *  runs the bootloader processing routine until instructed to soft-exit, or hard-reset via the watchdog to start
 *  the loaded application code.
 */
int main(void)
{
  /* Configure hardware required by the bootloader */
  SetupHardware();
  
  /* Run the USB management task while the bootloader is supposed to be running */
  while (1)
    USB_USBTask();
}

/** Configures all hardware required for the bootloader. */
void SetupHardware(void)
{
  /* Disable watchdog */
  MCUSR &= ~_BV(WDRF); // Clear Watchdog Reset Flag
  wdt_disable(); // Disable Watchdog Timer

  /* Enable global interrupts so that the USB stack can function */
  sei();

  /* Change the clock frequency prescaler */
  CLKPR = ((uint16_t)1<<7) | 16;

  /* Relocate the interrupt vector table to the bootloader section */
  MCUCR = _BV(IVCE);  // The IVCE bit must be written to logic one to enable change of the IVSEL bit
  MCUCR = _BV(IVSEL); // Move the Interrupt Vectors to the beginning of the Boot Loader section of the Flash

  /* Protocol initialization */
  USB_Init();
  SPI_Init(SPI_SPEED_FCPU_DIV_2 | SPI_ORDER_MSB_FIRST | SPI_SCK_LEAD_FALLING | SPI_SAMPLE_TRAILING | SPI_MODE_MASTER);

  /* Initialize the Dataflash */
  Dataflash_DeselectChip();
}

/** Resets all configured hardware required for the bootloader back to their original states. */
void ResetHardware(void)
{
  /* Relocate the interrupt vector table back to the application section */
  MCUCR = _BV(IVCE); // The IVCE bit must be written to logic one to enable change of the IVSEL bit
  MCUCR = 0        ; // Move the Interrupt Vectors to the beginning of the start of the Flash

  /* Shut down protocols */
  USB_ShutDown();
  SPI_ShutDown();
}

/** Routine to process an issued command from the host, via a DFU_DNLOAD request wrapper. This routine ensures
 *  that the command is allowed based on the current secure mode flag value, and passes the command off to the
 *  appropriate handler function.
 */
void ProcessFlipCommand()
{
  switch (flipCommand.group) {
    case CMD_GROUP_DOWNLOAD:
      ProcessDownload();
      break;
    case CMD_GROUP_UPLOAD:
      ProcessUpload();
      break;
    case CMD_GROUP_EXEC:
      ProcessExec();
      break;
    case CMD_GROUP_READ:
      ProcessRead();
      break;
    case CMD_GROUP_SELECT:
      ProcessSelect();
      break;
  }
}

/** Handler for a Memory Program command issued by the host. This routine handles the preparations needed
 *  to write subsequent data from the host into the specified memory.
 */
void ProcessDownload(void)
{
  if(flipCommand.data[0] == 0x00){ // Init FLASH programming
    /* Enter download mode if in dfuIDLE, if not in dfuDNLOAD_IDLE then enter dfuERROR */
    if(DFU_State != dfuIDLE){
      DFU_State = dfuERROR;
      return;
    }
    else{
      uint16_t startAddr = ((uint16_t)flipCommand.data[1] << 8) | (uint16_t)flipCommand.data[2];
      uint16_t endAddr   = ((uint16_t)flipCommand.data[3] << 8) | (uint16_t)flipCommand.data[4];
      uint16_t curAddr   = startAddr;

      /* Start downloading the firmware */
      while(DFU_State != dfuMANIFEST_SYNC){

        /* Wait for the OUT packet */
        while(!Endpoint_IsOUTReceived()){};

        /* Packet received, start reading the payload */
        DFU_State = dfuDNBUSY;

        for(uint8_t i=0;i<FIXED_CONTROL_ENDPOINT_SIZE;i+=2,curAddr+=2){

          /* See if it's a new page, if so we commit for the last page and erase the current one */
          if (curAddr%SPM_PAGESIZE==0) {
            
            /* Erase the current page */
            boot_page_erase(curAddr); boot_spm_busy_wait();

            /* Commit the last page */
            if(curAddr!=startAddr)
              boot_page_write(curAddr-2); boot_spm_busy_wait();

            /* Re-enable the RWW section of flash as writing to the flash locks it out */
            boot_rww_enable();

            /* This packet has been fully downloaded, change the state */
            if(curAddr > endAddr){
              DFU_State = dfuMANIFEST_SYNC;
              break;
            }
          }

          /* Write the next word into the current flash page */
          boot_page_fill(curAddr, Endpoint_Read_Word_LE());
        }

        /* Finished this packet, ack the host */
        Endpoint_ClearOUT(); 

        /* change the state and wait for the host to solicit the status via DFU_GETSTATUS. */
        if(DFU_State == dfuDNBUSY)
          DFU_State = dfuDNLOAD_SYNC;
      }
    }
  }
  else if(flipCommand.data[0] == 0x01){ // Init EEPROM programming
    /* Enter download mode if in dfuIDLE, if not in dfuDNLOAD_IDLE then enter dfuERROR */
    if(DFU_State != dfuIDLE){
      DFU_State = dfuERROR;
      return;
    }
    else{
      uint16_t startAddr = ((uint16_t)flipCommand.data[1] << 8) | (uint16_t)flipCommand.data[2];
      uint16_t endAddr   = ((uint16_t)flipCommand.data[3] << 8) | (uint16_t)flipCommand.data[4];
      uint16_t curAddr   = startAddr;

      /* Start downloading the EEPROM data */
      while(DFU_State != dfuMANIFEST_SYNC){

        /* Wait for the OUT packet */
        while(!Endpoint_IsOUTReceived()){};

        /* Packet received, start reading the payload */
        DFU_State = dfuDNBUSY;

        for(uint8_t i=0;i<FIXED_CONTROL_ENDPOINT_SIZE;i++,curAddr++){

          /* This packet has been fully downloaded, change the state */
          if(curAddr > endAddr){
            DFU_State = dfuMANIFEST_SYNC;
            break;
          }

          /* Read the byte from the USB interface and write to to the EEPROM */
          eeprom_write_byte((uint8_t*)curAddr, Endpoint_Read_Byte());
          eeprom_busy_wait();
        }

        /* Finished this packet, ack the host */
        Endpoint_ClearOUT(); 

        /* change the state and wait for the host to solicit the status via DFU_GETSTATUS. */
        if(DFU_State == dfuDNBUSY)
          DFU_State = dfuDNLOAD_SYNC;
      }
    }
  }
  else if(flipCommand.data[0] == 0x10){ // Init External Dataflash programming
    /* Enter download mode if in dfuIDLE, if not in dfuDNLOAD_IDLE then enter dfuERROR */
    if(DFU_State != dfuIDLE){
      DFU_State = dfuERROR;
      return;
    }
    else{
      uint32_t startAddr = ((uint32_t)curFlash64KBPageNumber << 16) | ((uint32_t)flipCommand.data[1] << 8) | (uint32_t)flipCommand.data[2];
      uint32_t endAddr   = ((uint32_t)curFlash64KBPageNumber << 16) | ((uint32_t)flipCommand.data[3] << 8) | (uint32_t)flipCommand.data[4];
      uint32_t curAddr   = startAddr;

      /* Since we only have one dataflash, we always enable CHIP1 */
      Dataflash_SelectChip(DATAFLASH_CHIP1);

      /* Enter buffer 1 write mode */
      Dataflash_Configure_Write_Page_Offset(DF_CMD_BUFF1WRITE, curAddr/DATAFLASH_PAGE_SIZE, curAddr%DATAFLASH_PAGE_SIZE);

      /* Start downloading the firmware */
      while(DFU_State != dfuMANIFEST_SYNC){

        /* Wait for the OUT packet */
        while(!Endpoint_IsOUTReceived()){};

        /* Packet received, start reading the payload */
        DFU_State = dfuDNBUSY;

        /* Start receiving the firmware */
        for(uint8_t i=0;i<FIXED_CONTROL_ENDPOINT_SIZE;i++,curAddr++){

          /* See if we've finished a page, if so we commit for the page */
          if (curAddr!=startAddr && curAddr%DATAFLASH_PAGE_SIZE==0) {
            /* Write the Dataflash buffer contents back to the Dataflash page */
            Dataflash_ToggleSelectedChipCS();
            Dataflash_Configure_Write_Page_Offset(DF_CMD_BUFF1TOMAINMEMWITHERASE, (curAddr/DATAFLASH_PAGE_SIZE)-1, 0);
            Dataflash_ToggleSelectedChipCS();
            Dataflash_WaitWhileBusy();

            /* This packet has been fully downloaded */
            if(curAddr > endAddr){
              /* Deselect the dataflash */
              Dataflash_DeselectChip();

              /* Change the state */
              DFU_State = dfuMANIFEST_SYNC;
              break;
            } else { /* Return to buffer 1 write mode */
              Dataflash_Configure_Write_Page_Offset(DF_CMD_BUFF1WRITE, curAddr/DATAFLASH_PAGE_SIZE, 0);
            }
          }

          /* Write the next word into the current flash page */
          Dataflash_SendByte(Endpoint_Read_Byte());
        }

        /* Finished this packet, ack the host */
        Endpoint_ClearOUT(); 

        /* change the state and wait for the host to solicit the status via DFU_GETSTATUS. */
        if(DFU_State == dfuDNBUSY)
          DFU_State = dfuDNLOAD_SYNC;
      }
    }
  }
}

/** Handler for a Memory Read command issued by the host. This routine handles the preparations needed
 *  to read subsequent data from the specified memory out to the host, as well as implementing the memory
 *  blank check command.
 */
void ProcessUpload(void)
{
  if (flipCommand.data[0] == 0x00) { // Display FLASH Data
    /* Enter download mode if in dfuIDLE, if not in dfuDNLOAD_IDLE then enter dfuERROR */
    if(DFU_State != dfuIDLE){
      DFU_State = dfuERROR;
      return;
    }
    else{
      uint16_t startAddr = ((uint16_t)flipCommand.data[1] << 8) | (uint16_t)flipCommand.data[2];
      uint16_t endAddr   = ((uint16_t)flipCommand.data[3] << 8) | (uint16_t)flipCommand.data[4];
      uint16_t curAddr   = startAddr;

      /* Change the state */
      DFU_State = dfuUPLOAD_IDLE;

      /* Start uploading the data */
      while(curAddr < endAddr){

        /* Wait for the IN Ready */
        while(!Endpoint_IsINReady()){};

        /* Write the next word into the endpoint */
        for(uint8_t i=0;i<FIXED_CONTROL_ENDPOINT_SIZE;i+=2,curAddr+=2)
          Endpoint_Write_Word_LE(pgm_read_word(curAddr));

        /* Finished this packet, ack the host */
        Endpoint_ClearIN(); 
      }
    }
  }
  else if (flipCommand.data[0] == 0x01) { // Blank Check in FLASH
    uint16_t startAddr = ((uint16_t)flipCommand.data[1] << 8) | (uint16_t)flipCommand.data[2];
    uint16_t endAddr   = ((uint16_t)flipCommand.data[3] << 8) | (uint16_t)flipCommand.data[4];
    uint16_t curAddr=startAddr;

    for(curAddr=startAddr;curAddr<endAddr;curAddr++){
      if (pgm_read_byte(curAddr) != 0xFF) { // Found a non-blank byte
        DFU_State  = dfuERROR;
        DFU_Status = errCHECK_ERASED;
        nonBlankAddr = curAddr;
        break;
      }
    }
  }
  else if (flipCommand.data[0] == 0x02) { // Display EEPROM Data
    /* Enter download mode if in dfuIDLE, if not in dfuDNLOAD_IDLE then enter dfuERROR */
    if(DFU_State != dfuIDLE){
      DFU_State = dfuERROR;
      return;
    }
    else{
      uint16_t startAddr = ((uint16_t)flipCommand.data[1] << 8) | (uint16_t)flipCommand.data[2];
      uint16_t endAddr   = ((uint16_t)flipCommand.data[3] << 8) | (uint16_t)flipCommand.data[4];
      uint16_t curAddr   = startAddr;

      /* Change the state */
      DFU_State = dfuUPLOAD_IDLE;

      /* Start uploading the data */
      while(curAddr < endAddr){

        /* Wait for the IN Ready */
        while(!Endpoint_IsINReady()){};

        /* Read the EEPROM byte and send it via USB to the host */
        for(uint8_t i=0;i<FIXED_CONTROL_ENDPOINT_SIZE;i++,curAddr++)
          Endpoint_Write_Byte(eeprom_read_byte((uint8_t*)curAddr));

        /* Finished this packet, ack the host */
        Endpoint_ClearIN(); 
      }
    }
  }
  //else if (flipCommand.data[0] == 0x03) { // Blank Check in EEPROM 
  //  uint16_t startAddr = ((uint16_t)flipCommand.data[1] << 8) | (uint16_t)flipCommand.data[2];
  //  uint16_t endAddr   = ((uint16_t)flipCommand.data[3] << 8) | (uint16_t)flipCommand.data[4];
  //  uint16_t curAddr=startAddr;
  //  for(curAddr=startAddr;curAddr<endAddr;curAddr++){
  //    if (eeprom_read_byte((uint8_t*)curAddr) != 0xFF) { // Found a non-blank byte
  //      DFU_State  = dfuERROR;
  //      DFU_Status = errCHECK_ERASED;
  //      nonBlankAddr = curAddr;
  //      break;
  //    }
  //  }
  //}
  else if(flipCommand.data[0] == 0x10){ // Display External Dataflash Data 
    /* Enter download mode if in dfuIDLE, if not in dfuDNLOAD_IDLE then enter dfuERROR */
    if(DFU_State != dfuIDLE){
      DFU_State = dfuERROR;
      return;
    }
    else{
      uint32_t startAddr = ((uint32_t)curFlash64KBPageNumber << 16) | ((uint32_t)flipCommand.data[1] << 8) | (uint32_t)flipCommand.data[2];
      uint32_t endAddr   = ((uint32_t)curFlash64KBPageNumber << 16) | ((uint32_t)flipCommand.data[3] << 8) | (uint32_t)flipCommand.data[4];
      uint32_t curAddr   = startAddr;

      /* Change the state */
      DFU_State = dfuUPLOAD_IDLE;

      /* Since we only have one dataflash, we always enable CHIP1 */
      Dataflash_SelectChip(DATAFLASH_CHIP1);

      /* Enter buffer 1 write mode */
      Dataflash_Configure_Read_Page_Offset(DF_CMD_CONTARRAYREAD_LF, curAddr/DATAFLASH_PAGE_SIZE, curAddr%DATAFLASH_PAGE_SIZE);

      /* Start uploading the data */
      while(curAddr < endAddr){

        /* Wait for the IN Ready */
        while(!Endpoint_IsINReady()){};

        /* Write the next word int the endpoint */
        for(uint8_t i=0;i<FIXED_CONTROL_ENDPOINT_SIZE;i++,curAddr++)
          Endpoint_Write_Byte(Dataflash_ReceiveByte());

        /* Finished this packet, ack the host */
        Endpoint_ClearIN(); 
      }

      /* Deselect the dataflash */
      Dataflash_DeselectChip();
    }
  }
  else if (flipCommand.data[0] == 0x11) { // Blank Check in Dataflash 
    uint32_t startAddr = ((uint32_t)curFlash64KBPageNumber << 16) | ((uint32_t)flipCommand.data[1] << 8) | (uint32_t)flipCommand.data[2];
    uint32_t endAddr   = ((uint32_t)curFlash64KBPageNumber << 16) | ((uint32_t)flipCommand.data[3] << 8) | (uint32_t)flipCommand.data[4];
    uint32_t curAddr   = startAddr;

    /* Since we only have one dataflash, we always enable CHIP1 */
    Dataflash_SelectChip(DATAFLASH_CHIP1);

    /* Enter buffer 1 write mode */
    Dataflash_Configure_Read_Page_Offset(DF_CMD_CONTARRAYREAD_LF, curAddr/DATAFLASH_PAGE_SIZE, curAddr%DATAFLASH_PAGE_SIZE);

    /* Start uploading the data */
    for(curAddr=startAddr;curAddr<endAddr;curAddr++){
      if (Dataflash_ReceiveByte() != 0xFF) { // Found a non-blank byte
        DFU_State  = dfuERROR;
        DFU_Status = errCHECK_ERASED;
        nonBlankAddr = curAddr;
        break;
      }
    }

    /* Deselect the dataflash */
    Dataflash_DeselectChip();
  }
}

/** Handler for a Data Write command issued by the host. This routine handles non-programming commands such as
 *  bootloader exit (both via software jumps and hardware watchdog resets) and flash memory erasure.
 */
void ProcessExec(void)
{
  if (flipCommand.data[0] == 0x00 && flipCommand.data[1] == 0xFF) { // Erase flash
    /* Clear the application section of flash */
    for(uint16_t curAddr=0;curAddr<BOOT_START_ADDR;curAddr+=SPM_PAGESIZE) {
      boot_page_erase(curAddr); boot_spm_busy_wait();
    }
    /* Re-enable the RWW section of flash as writing to the flash locks it out */
    boot_rww_enable();
  }
  if (flipCommand.data[0] == 0x01 && flipCommand.data[1] == 0xFF) { // Erase eeprom 
    for(uint16_t curAddr=0;curAddr<512;curAddr++) {
      eeprom_write_byte((uint8_t*)curAddr, 0xFF);
      eeprom_busy_wait();
    }
  }
  else if (flipCommand.data[0] == 0x10 && flipCommand.data[1] == 0xFF) { // Erase External Flash 
    Dataflash_SelectChip(DATAFLASH_CHIP1);
    Dataflash_SendByte(0xC7);
    Dataflash_SendByte(0x94);
    Dataflash_SendByte(0x80);
    Dataflash_SendByte(0x9A);
    Dataflash_ToggleSelectedChipCS();
    Dataflash_WaitWhileBusy();
    Dataflash_DeselectChip();
  }
  else if (flipCommand.data[0] == 0x01){ // Set configuration
  }
  else if (flipCommand.data[0] == 0x03){ // Start application
    if (flipCommand.data[1] == 0x00) { // Start via watchdog
      /* Start the watchdog to reset the AVR once the communications are finalized */
      wdt_enable(WDTO_250MS);
    } 
    else if (flipCommand.data[1] == 0x01) { // Start via jump
      /* Load in the jump address into the application start address pointer */
      AppStartPtr = ((uint16_t)flipCommand.data[3] << 8) | (uint16_t)flipCommand.data[4];
    }
  }
}

/** Handler for reading configuration information or manufacturer information.
 */
void ProcessRead(void)
{
  while(!Endpoint_IsINReady()){};

  switch (flipCommand.data[0])
  {
    case 0x00: // Read bootloader info
      switch (flipCommand.data[1])
      {
        case 0x00: Endpoint_Write_Byte(BOOTLOADER_VERSION) ; break;
        case 0x01: Endpoint_Write_Byte(BOOTLOADER_ID_BYTE1); break;
        case 0x02: Endpoint_Write_Byte(BOOTLOADER_ID_BYTE2); break;
      }
      break;
    case 0x01: // Read device info
      switch (flipCommand.data[1])
      {
        case 0x30: Endpoint_Write_Byte(MANUFACTURER_CODE); break;
        case 0x31: Endpoint_Write_Byte(FAMILY_CODE)      ; break;
        case 0x60: Endpoint_Write_Byte(PRODUCT_NAME)     ; break;
        case 0x61: Endpoint_Write_Byte(PRODUCT_REVISION) ; break;
      }
      break;
  }

  Endpoint_ClearIN(); 
}

/** Handler for a Change Base Address command issued by the host. 
 * 
 */
void ProcessSelect(void)
{
  if (flipCommand.data[0] == 0x03){
    if (flipCommand.data[1] == 0x00) // Select Memory Page 
      curFlash64KBPageNumber = flipCommand.data[2];
  }
}

void UpdateState(void)
{
  switch (DFU_State)
  {
    case dfuDNLOAD_SYNC:
      DFU_State = dfuDNLOAD_IDLE; break;
    case dfuUPLOAD_IDLE:
      DFU_State = dfuIDLE; break;
    case dfuMANIFEST_SYNC:
      DFU_State = dfuIDLE; break;
  }
}

void EVENT_USB_Device_UnhandledControlRequest(void)
{
  /* Send ACK */
  Endpoint_ClearSETUP();

  switch (USB_ControlRequest.bRequest)
  {
    case DFU_DETACH:
      break;

    case DFU_DNLOAD:
      /* Check if there's a FLIP command */
      if(USB_ControlRequest.wLength){
        /* Wait for the packet */
        while(!Endpoint_IsOUTReceived()){};

        /* Retrieve the FLIP command */
        flipCommand.group   = Endpoint_Read_Byte();
        for(uint8_t i=0;i<5 && i<(USB_ControlRequest.wLength-1);i++)
          flipCommand.data[i] = Endpoint_Read_Byte();
        Endpoint_ClearOUT();

        /* If wLength is not 6 then it's a downlaod command, we discard the paddings and process it */
        if(
            (flipCommand.group == CMD_GROUP_DOWNLOAD) ||        
            (flipCommand.group == CMD_GROUP_UPLOAD && flipCommand.data[0] == 0x01) || // Flash blank check
            (flipCommand.group == CMD_GROUP_UPLOAD && flipCommand.data[0] == 0x03) || // EEPROM blank check
            (flipCommand.group == CMD_GROUP_UPLOAD && flipCommand.data[0] == 0x11) || // Dataflash blank check
            (flipCommand.group == CMD_GROUP_EXEC) ||
            (flipCommand.group == CMD_GROUP_SELECT)
          )
          waitForSecondRequest = false;
        else
          waitForSecondRequest = true;

        /* Process the command if not waiting for the second request */
        if(!waitForSecondRequest)
          ProcessFlipCommand();
      }
      /* DFU_DNLOAD with no data means it's a terminating signal */
      else {
        ResetHardware();
        /* Start the user application */
        AppStartPtr();
      }
      break;

    case DFU_UPLOAD:

      /* Blank checking is performed in the DFU_DNLOAD request - if we get here we've told the host
         that the memory isn't blank, and the host is requesting the first non-blank address */
      if (
           (flipCommand.group == CMD_GROUP_UPLOAD && flipCommand.data[0] == 0x01) || // Flash blank check
           (flipCommand.group == CMD_GROUP_UPLOAD && flipCommand.data[0] == 0x03) || // EEPROM blank check
           (flipCommand.group == CMD_GROUP_UPLOAD && flipCommand.data[0] == 0x11)    // Dataflash blank check
         ) {
        /* Wait for the IN Ready */
        while(!Endpoint_IsINReady()){};

        /* Write the first non-blank address */
        Endpoint_Write_Word_LE((uint16_t)nonBlankAddr);

        /* Finished this packet, ack the host */
        Endpoint_ClearIN(); 

        break;
      }

      /* We have received the command through the last DFU_DNLOAD, process it directly */
      ProcessFlipCommand();

    case DFU_GETSTATUS:
      /* Update the state */
      UpdateState();
      /* Wait for the IN Ready */
      while(!Endpoint_IsINReady()){};
      /* 1 byte status value */
      Endpoint_Write_Byte(DFU_Status);
      /* 3 byte poll timeout value */
      Endpoint_Write_Byte(0);
      Endpoint_Write_Byte(0);
      Endpoint_Write_Byte(0);
      /* 1 byte status value */
      Endpoint_Write_Byte(DFU_State);
      /* 1 byte state string ID number */
      Endpoint_Write_Byte(0);
      Endpoint_ClearIN();
      break;    

    case DFU_CLRSTATUS:

      DFU_State = dfuIDLE;
      DFU_Status = OK;
      break;

    case DFU_GETSTATE:

      /* Wait for the IN Ready */
      while(!Endpoint_IsINReady()){};
      Endpoint_Write_Byte(DFU_State);
      Endpoint_ClearIN();
      break;

    case DFU_ABORT:

      DFU_State = dfuIDLE;
      DFU_Status = OK;
      break;
  }
  Endpoint_ClearStatusStage();
}

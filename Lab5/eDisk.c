// eDisk.c
// Runs on TM4C123
// Mid-level implementation of the solid state disk device
// driver.  Below this is the low level, hardware-specific
// flash memory interface.  Above this is the high level
// file system implementation.
// Daniel and Jonathan Valvano
// August 29, 2016

/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2016

   "Embedded Systems: Real-Time Operating Systems for ARM Cortex-M Microcontrollers",
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2016

   "Embedded Systems: Introduction to the MSP432 Microcontroller",
   ISBN: 978-1512185676, Jonathan Valvano, copyright (c) 2016

   "Embedded Systems: Real-Time Interfacing to the MSP432 Microcontroller",
   ISBN: 978-1514676585, Jonathan Valvano, copyright (c) 2016

 Copyright 2016 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

#include <stdint.h>
#include "eDisk.h"
#include "FlashProgram.h"

//*************** eDisk_Init ***********
// Initialize the interface between microcontroller and disk
// Inputs: drive number (only drive 0 is supported)
// Outputs: status
//  RES_OK        0: Successful
//  RES_ERROR     1: Drive not initialized
enum DRESULT eDisk_Init(uint32_t drive){
  // if drive is 0, return RES_OK, otherwise return RES_ERROR
  // for some configurations the physical drive needs initialization
  // however for the internal flash, no initialization is required
  //    so this function doesn't do anything
  if(drive == 0){             // only drive 0 is supported
     return RES_OK;
  }
  return RES_ERROR;

}

//*************** eDisk_ReadSector ***********
// Read 1 sector of 512 bytes from the disk, data goes to RAM
// Inputs: pointer to an empty RAM buffer
//         sector number of disk to read: 0,1,2,...255
// Outputs: result
//  RES_OK        0: Successful
//  RES_ERROR     1: R/W Error
//  RES_WRPRT     2: Write Protected
//  RES_NOTRDY    3: Not Ready
//  RES_PARERR    4: Invalid Parameter
enum DRESULT eDisk_ReadSector(
    uint8_t *buff,     // Pointer to a RAM buffer into which to store
    uint8_t sector){   // sector number to read from
// starting ROM address of the sector is	EDISK_ADDR_MIN + 512*sector
// return RES_PARERR if EDISK_ADDR_MIN + 512*sector > EDISK_ADDR_MAX
// copy 512 bytes from ROM (disk) into RAM (buff)
// **write this function**

    // Step 1: Get address pt
    volatile uint32_t *read_address_pt;
    read_address_pt = (volatile uint32_t *) (EDISK_ADDR_MIN + (512 * sector));
    // Test if address is out of bounds
    if (read_address_pt > (uint32_t *) EDISK_ADDR_MAX) {
        return RES_PARERR;
    }
    // Step 2: Read memory contents 8bits (1 byte) at a time
    uint8_t *read_pt = (uint8_t *)read_address_pt;
    for (int i = 0; i < 512; i++) {
        buff[i] = *(read_pt);
        read_pt +=1;
    }
    return RES_OK;
}

//*************** eDisk_WriteSector ***********
// Write 1 sector of 512 bytes of data to the disk, data comes from RAM
// Inputs: pointer to RAM buffer with information
//         sector number of disk to write: 0,1,2,...,255
// Outputs: result
//  RES_OK        0: Successful
//  RES_ERROR     1: R/W Error
//  RES_WRPRT     2: Write Protected
//  RES_NOTRDY    3: Not Ready
//  RES_PARERR    4: Invalid Parameter
enum DRESULT eDisk_WriteSector(
    const uint8_t *buff,  // Pointer to the data to be written
    uint8_t sector){      // sector number
// starting ROM address of the sector is	EDISK_ADDR_MIN + 512*sector
// return RES_PARERR if EDISK_ADDR_MIN + 512*sector > EDISK_ADDR_MAX
// write 512 bytes from RAM (buff) into ROM (disk)
// you can use Flash_FastWrite or Flash_WriteArray
// **write this function**
        
    // Step 1: Get write_address
    uint32_t write_address;
    write_address = (uint32_t) (EDISK_ADDR_MIN + (512 * sector));
    // Test if address is out of bounds
    if (write_address > (uint32_t) EDISK_ADDR_MAX) {
        return RES_PARERR;
    }

    // Step 2: Allign 8 bit values to 32bit values to use FLASH_WriteArray()
    uint32_t alligned_32bit_data[128];
    uint32_t index_32bit = 0;
    for (int i = 0; i < 512; i = i + 4) {
        alligned_32bit_data[index_32bit] = 0;
        // Byte 1
        alligned_32bit_data[index_32bit] += buff[i+3];
        alligned_32bit_data[index_32bit] = (alligned_32bit_data[index_32bit] << 8);
        // Byte 2
        alligned_32bit_data[index_32bit] += buff[i+2];
        alligned_32bit_data[index_32bit] = (alligned_32bit_data[index_32bit] << 8);
        // Byte 3
        alligned_32bit_data[index_32bit] += buff[i+1];
        alligned_32bit_data[index_32bit] = (alligned_32bit_data[index_32bit] << 8);
        // Byte 4
        alligned_32bit_data[index_32bit] += buff[i];
        index_32bit++;
    }
    
    // Step 3: Write to flash
    int status;
    status = Flash_WriteArray(alligned_32bit_data, write_address, 128);
    // Flash_Write_Array returns number of succesful writes
    if (status == 128) {
        return RES_OK;
    } else {
        return RES_ERROR;
    }
}

//*************** eDisk_Format ***********
// Erase all files and all data by resetting the flash to all 1's
// Inputs: none
// Outputs: result
//  RES_OK        0: Successful
//  RES_ERROR     1: R/W Error
//  RES_WRPRT     2: Write Protected
//  RES_NOTRDY    3: Not Ready
//  RES_PARERR    4: Invalid Parameter
enum DRESULT eDisk_Format(void){
// erase all flash from EDISK_ADDR_MIN to EDISK_ADDR_MAX
// **write this function**
  int status;
  uint32_t erase_address = (uint32_t) EDISK_ADDR_MIN; // start of disk
  while(erase_address < (uint32_t) EDISK_ADDR_MAX) {
    status = Flash_Erase(erase_address); // erase 1k block
    erase_address = erase_address + 1024;
  }
  if (status == RES_OK) {
    return RES_OK;
  } else {
      return RES_ERROR;
  }
}

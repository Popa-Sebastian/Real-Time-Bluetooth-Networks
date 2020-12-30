// eFile.c
// Runs on either TM4C123 or MSP432
// High-level implementation of the file system implementation.
// Daniel and Jonathan Valvano
// August 29, 2016
#include <stdint.h>
#include "eDisk.h"

uint8_t Buff[512]; // temporary buffer used during file I/O
uint8_t Directory[256], FAT[256];
int32_t bDirectoryLoaded =0; // 0 means disk on ROM is complete, 1 means RAM version active

// Return the larger of two integers.
int16_t max(int16_t a, int16_t b){
  if(a > b){
    return a;
  }
  return b;
}
//*****MountDirectory******
// if directory and FAT are not loaded in RAM,
// bring it into RAM from disk
void MountDirectory(void){ 
// if bDirectoryLoaded is 0, 
//    read disk sector 255 and populate Directory and FAT
//    set bDirectoryLoaded=1
// if bDirectoryLoaded is 1, simply return
// **write this function**
    int i;
    uint8_t buff[512];
    if (bDirectoryLoaded == 0) {
        eDisk_ReadSector (buff, 255);
        // Get Directory
        for (i = 0; i< 256; i++) {
            Directory[i] = buff[i];
        }
        // Get FAT
        for (i = 256; i < 512 ; i++) {
            FAT[i - 256] = buff[i];
        }
        // Set bDirectoryLoaded
        bDirectoryLoaded = 1;
    }
    // else return
}


// Return the index of the last sector in the file
// associated with a given starting sector.
// Note: This function will loop forever without returning
// if the file has no end (i.e. the FAT is corrupted).
uint8_t lastsector(uint8_t start){
// **write this function**
    uint8_t index_value;
    uint8_t current_index;
    if (start == 255) {
      return 255;
    } else {
      current_index = start;
      while (1) {
          index_value = FAT[current_index];
          if (index_value == 255) {
              return current_index;
          } else {
              current_index = index_value;
          }
      }
    }
}

// Return the index of the first free sector.
// Note: This function will loop forever without returning
// if a file has no end or if (Directory[255] != 255)
// (i.e. the FAT is corrupted).
uint8_t findfreesector(void){
// **write this function**
    int free_sector = -1;
    uint8_t index = 0;
    uint8_t last_sector;
    while (1) {
        last_sector = lastsector(Directory[index]);
        if (last_sector == 255) { // no more files or no files in Directory
            // free sector is the next one after the greatest last sector
            return free_sector + 1; 
        } else {
            free_sector = max(free_sector, last_sector);
            index++;
        }
    }
}

// Append a sector index 'n' at the end of file 'num'.
// This helper function is part of OS_File_Append(), which
// should have already verified that there is free space,
// so it always returns 0 (successful).
// Note: This function will loop forever without returning
// if the file has no end (i.e. the FAT is corrupted).
uint8_t appendfat(uint8_t num, uint8_t n){
// **write this function**
    uint8_t index;
    index = Directory[num];
    if (index == 255){
        Directory[num] = n; // this is the first sector of a new file
    } else {
        while (1) {
            if (FAT[index] == 255) {
                FAT[index] = n; // append sector number to end
                return 0;
            } else {
                index = FAT[index]; // keep searching for end
            }
        }
    }
    return 0; // success
}

//********OS_File_New*************
// Returns a file number of a new file for writing
// Inputs: none
// Outputs: number of a new file
// Errors: return 255 on failure or disk full
uint8_t OS_File_New(void){
// **write this function**
    uint8_t index = 0;
    if (bDirectoryLoaded == 0) {
        MountDirectory();
    }
    while (index != 255) { // go through Directory and find a free space
        if (Directory[index] == 255) {
            return index;
        } else {
            index++;
        }
    }
    return 255; // disk is full
}

//********OS_File_Size*************
// Check the size of this file
// Inputs:  num, 8-bit file number, 0 to 254
// Outputs: 0 if empty, otherwise the number of sectors
// Errors:  none
uint8_t OS_File_Size(uint8_t num){
// **write this function**
    uint8_t index;
    uint8_t size = 0;
    if (Directory[num] == 255) {
        return 0; // this is a new file
    } else {
        index = Directory[num]; // start of file
        while (size < 255) {
            size++;
            if (FAT[index] == 255) { //eof
                return size;
            } else {                 // continue
                index = FAT[index];
            }
        }
    }
    return 255;
}

//********OS_File_Append*************
// Save 512 bytes into the file
// Inputs:  num, 8-bit file number, 0 to 254
//          buf, pointer to 512 bytes of data
// Outputs: 0 if successful
// Errors:  255 on failure or disk full
uint8_t OS_File_Append(uint8_t num, uint8_t buf[512]){
// **write this function**
    if (bDirectoryLoaded == 0) {
        MountDirectory();
    }
    uint8_t sector = findfreesector();
    if (sector == 255) {
        return 255;
    } else {
        eDisk_WriteSector(buf, sector);
        appendfat(num, sector);
        return 0;
    }
}

//********OS_File_Read*************
// Read 512 bytes from the file
// Inputs:  num, 8-bit file number, 0 to 254
//          location, logical address, 0 to 254
//          buf, pointer to 512 empty spaces in RAM
// Outputs: 0 if successful
// Errors:  255 on failure because no data
uint8_t OS_File_Read(uint8_t num, uint8_t location,
                     uint8_t buf[512]){
// **write this function**
    uint8_t sector; 
    sector = Directory[num];
    if (sector == 255){
        return 255;
    }
    for (int i = 0; i < location; i++){
        sector = FAT[sector];
        if(sector == 255){
            return 255;
        }
    }
    return eDisk_ReadSector(buf, sector);
}

//********OS_File_Flush*************
// Update working buffers onto the disk
// Power can be removed after calling flush
// Inputs:  none
// Outputs: 0 if success
// Errors:  255 on disk write failure
uint8_t OS_File_Flush(void){
// **write this function**
    int i;
    int status;
    uint8_t buff[512];
    for (i = 0; i < 256; i++) {
        buff[i] = Directory[i];
    }
    for (i = 256; i < 512; i++) {
        buff[i] = FAT[i - 256];
    }
    status = eDisk_WriteSector(buff, 255);
    return status;
}

//********OS_File_Format*************
// Erase all files and all data
// Inputs:  none
// Outputs: 0 if success
// Errors:  255 on disk write failure
uint8_t OS_File_Format(void){
// call eDiskFormat
// clear bDirectoryLoaded to zero
// **write this function**
    int status;
    status = eDisk_Format();
    bDirectoryLoaded = 0;
    return status; // replace this line
}

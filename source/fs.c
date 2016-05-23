/*
*   fs.c
*/

#include "fs.h"
#include "fatfs/ff.h"

static FATFS fs;

u32 mountSD(void){
    if(f_mount(&fs, "0:", 1) != FR_OK) return 0;
    return 1;
}

u32 fileWrite(void *orig, const char *path, u32 size){
    FRESULT fr;
    FIL fp;
    unsigned int bw = 0;

    if(!size) return 0;
    fr = f_open(&fp, path, FA_WRITE | FA_CREATE_ALWAYS);
    if(fr == FR_OK){
        fr = f_write(&fp, orig, size, &bw);
    }

    f_close(&fp);
    return (fr || (bw != size)) ? 0 : 1;
}

u32 fileRead(void *dest, const char *path, u32 size){
    FRESULT fr;
    FIL fp;
    unsigned int br = 0;

    fr = f_open(&fp, path, FA_READ);
    if(fr == FR_OK){
        if(!size) size = f_size(&fp);
        fr = f_read(&fp, dest, size, &br);
    }

    f_close(&fp);
    return fr ? 0 : 1;
}

u32 fileSize(const char *path){
    FIL fp;
    u32 size = 0;

    if(f_open(&fp, path, FA_READ) == FR_OK)
        size = f_size(&fp);

    f_close(&fp);
    return size;
}
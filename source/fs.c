/*
*   fs.c
*/

#include "fs.h"
#include "memory.h"
#include "fatfs/ff.h"

static FATFS fs;

u32 mountSD(void)
{
    return f_mount(&fs, "0:", 1) == FR_OK;
}

u32 mountCTRNAND(void)
{
    return f_mount(&fs, "1:", 1) == FR_OK;
}

u32 fileRead(void *dest, const char *path)
{
    FIL file;
    u32 size;

    if(f_open(&file, path, FA_READ) == FR_OK)
    {
        unsigned int read;
        size = f_size(&file);
        f_read(&file, dest, size, &read);
        f_close(&file);
    }
    else size = 0;

    return size;
}

void fileWrite(const void *buffer, const char *path, u32 size)
{
    FIL file;

    if(f_open(&file, path, FA_WRITE | FA_OPEN_ALWAYS) == FR_OK)
    {
        unsigned int written;
        f_write(&file, buffer, size, &written);
        f_close(&file);
    }
}

u32 firmRead(void *dest)
{
    const char *firmFolders[] = { "00000002", "20000002" };
    char path[48] = "1:/title/00040138/00000000/content";
    memcpy(&path[18], firmFolders[console], 8);

    DIR dir;
    FILINFO info;

    f_opendir(&dir, path);

    u32 id = 0xFFFFFFFF,
        ret = 0;

    //Parse the target directory
    while(f_readdir(&dir, &info) == FR_OK && info.fname[0])
    {
        //Not a cxi
        if(info.altname[9] != 'A') continue;

        //Multiple cxis were found
        if(id != 0xFFFFFFFF) ret = 1;

        //Convert the .app name to an integer
        u32 tempId = 0;
        for(char *tmp = info.altname; *tmp != '.'; tmp++)
        {
            tempId <<= 4;
            tempId += *tmp > '9' ? *tmp - 'A' + 10 : *tmp - '0';
        }

        //FIRM is equal or newer than 11.0
        if(tempId >= (console ? 0x21 : 0x52)) ret = 2;

        //Found an older cxi
        if(tempId < id) id = tempId;
    }

    f_closedir(&dir);

    if(!ret)
    {
        //Complete the string with the .app name
        memcpy(&path[34], "/00000000.app", 14);

        //Last digit of the .app
        u32 i = 42;

        //Convert back the .app name from integer to array
        while(id)
        {
            static const char hexDigits[] = "0123456789ABCDEF";
            path[i--] = hexDigits[id & 0xF];
            id >>= 4;
        }

        fileRead(dest, path);
    }

    return ret;
}
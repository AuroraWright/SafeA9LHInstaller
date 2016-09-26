/*
*   utils.c
*/

#include "utils.h"
#include "draw.h"
#include "screen.h"
#include "cache.h"
#include "i2c.h"

u32 waitInput(void)
{
    bool pressedKey = false;
    u32 key;

    //Wait for no keys to be pressed
    while(HID_PAD);

    do
    {
        //Wait for a key to be pressed
        while(!HID_PAD);

        key = HID_PAD;

        //Make sure it's pressed
        for(u32 i = 0x13000; i > 0; i--)
        {
            if(key != HID_PAD) break;
            if(i == 1) pressedKey = true;
        }
    }
    while(!pressedKey);

    return key;
}

void shutdown(u32 mode, const char *message)
{
    if(mode)
    {
        posY = drawString(message, 10, posY + SPACING_Y, mode == 1 ? COLOR_RED : COLOR_GREEN);
        drawString("Press any button to shutdown", 10, posY, COLOR_WHITE);
        waitInput();
    }

    if(PDN_GPU_CNT != 1) clearScreens();
    flushEntireDCache();
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 0);
    while(true);
}
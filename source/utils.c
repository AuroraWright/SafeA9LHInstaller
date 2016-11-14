/*
*   utils.c
*/

#include "utils.h"
#include "draw.h"
#include "screen.h"
#include "cache.h"
#include "i2c.h"

static void startChrono(void)
{
    REG_TIMER_CNT(0) = 0; //67MHz
    for(u32 i = 1; i < 4; i++) REG_TIMER_CNT(i) = 4; //Count-up

    for(u32 i = 0; i < 4; i++) REG_TIMER_VAL(i) = 0;

    REG_TIMER_CNT(0) = 0x80; //67MHz; enabled
    for(u32 i = 1; i < 4; i++) REG_TIMER_CNT(i) = 0x84; //Count-up; enabled
}

static u64 chrono(void)
{
    u64 res;
    for(u32 i = 0; i < 4; i++) res |= REG_TIMER_VAL(i) << (16 * i);

    res /= (TICKS_PER_SEC / 1000);

    return res;
}

u32 waitInput(void)
{
    bool pressedKey = false;
    u32 key,
        oldKey = HID_PAD;

    while(!pressedKey)
    {
        key = HID_PAD;

        if(!key) oldKey = key;
        else if(key != oldKey)
        {
            //Make sure the key is pressed
            u32 i;
            for(i = 0; i < 0x13000 && key == HID_PAD; i++);
            if(i == 0x13000) pressedKey = true;
        }
    }

    return key;
}

void wait(u64 amount)
{
    startChrono();
    while(chrono() < amount);
}

void mcuReboot(void)
{
    clearScreens();

    //Ensure that all memory transfers have completed and that the data cache has been flushed
    flushEntireDCache();

    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
    while(true);
}

void inputSequence(void)
{
    posY = drawString("If you would like to continue, press:", 10, posY, COLOR_WHITE);
    posY = drawString("Up, Down, Left, Right, B, A, START, SELECT", 10, posY, COLOR_WHITE);

    u32 unlockSequence[] = { BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT, BUTTON_B, BUTTON_A, BUTTON_START, BUTTON_SELECT },
        sequenceSize = sizeof(unlockSequence) / sizeof(u32);

    for(u32 correctPresses = 0; correctPresses < sequenceSize; correctPresses++)
    {
        if(waitInput() != unlockSequence[correctPresses])
            shutdown(1, "Button sequence not entered correctly");
    }
}

void shutdown(u32 mode, const char *message)
{
    if(mode != 0)
    {
        posY = drawString(message, 10, posY + SPACING_Y, mode == 1 ? COLOR_RED : COLOR_GREEN);
        drawString("Press any button to shutdown", 10, posY, COLOR_WHITE);
        waitInput();
    }

    clearScreens();

    //Ensure that all memory transfers have completed and that the data cache has been flushed
    flushEntireDCache();

    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 0);
    while(true);
}
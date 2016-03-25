#include "utils.h"
#include "draw.h"
#include "i2c.h"

u16 waitInput(void){
    u32 pressedkey = 0;
    u16 key;

    //Wait for no keys to be pressed
    while(HID_PAD);

    do {
        //Wait for a key to be pressed
        while(!HID_PAD);
        key = HID_PAD;

        //Make sure it's pressed
        for(u32 i = 0x13000; i; i--){
            if (key != HID_PAD)
                break;
            if(i==1) pressedkey = 1;
        }
    } while (!pressedkey);

    return key;
}

void shutdown(u32 mode, char *message){
    if(mode){
        pos_y = drawString(message, 10, pos_y + SPACING_VERT, COLOR_RED);
        drawString("Press any button to shutdown", 10, pos_y, COLOR_WHITE);
        waitInput();
    }
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1);
    while(1);
}
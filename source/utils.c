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
        pos_y = drawString(message, 10, pos_y + SPACING_VERT, COLOR_WHITE);
        if(mode == 1) drawString("Press any button to shutdown", 10, pos_y, COLOR_WHITE);
        else {
            pos_y = drawString("Press START or SELECT to return to menu", 10, pos_y, COLOR_RED);
            pos_y = drawString("(SD will be unmounted until the next install)", 10, pos_y, COLOR_RED);
            pos_y = drawString("Press any other button to shutdown", 10, pos_y, COLOR_WHITE);
        }
        u16 pressed = waitInput();
        if(mode == 2 && (pressed & (BUTTON_START | BUTTON_SELECT))) return;
    }
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1);
    while(1);
}
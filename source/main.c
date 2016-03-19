/*
*   main.c
*       by Reisyukaku
*   Copyright (c) 2015 All Rights Reserved
*
*   Minimalist CFW for N3DS
*/

#include "fs.h"
#include "installer.h"
#include "screeninit.h"
#include "draw.h"
#include "types.h"

#define PDN_GPU_CNT        (*(vu8 *)0x10141200)

void main(void){
    mountSD();
    if(PDN_GPU_CNT == 0x1) initLCD();
    clearScreens();
    installer();
}
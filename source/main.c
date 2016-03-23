/*
*   main.c
*       by Reisyukaku
*   Copyright (c) 2015 All Rights Reserved
*
*   Minimalist CFW for N3DS
*/

#include "installer.h"
#include "screeninit.h"
#include "types.h"

void main(void){
    initScreens();
    installer();
}
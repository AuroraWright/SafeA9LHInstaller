/*
*   draw.h
*
*   Code to print to the screen by mid-kid @CakesFW
*/

#pragma once

#include "types.h"

#define SCREEN_TOP_WIDTH  400
#define SCREEN_TOP_HEIGHT 240

#define SPACING_Y 10
#define SPACING_X 8

void clearScreens(void);
void drawCharacter(char character, int pos_x, int pos_y, u32 color);
int drawString(const char *string, int pos_x, int pos_y, u32 color);
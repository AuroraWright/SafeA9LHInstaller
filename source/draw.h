#pragma once

#include "types.h"

#define SPACING_VERT 10
#define SPACING_HORIZ 8

void clearScreens();
void drawCharacter(char character, int pos_x, int pos_y, u32 color);
int drawString(const char *string, int pos_x, int pos_y, u32 color);
#pragma once

#include "types.h"

#define HID_PAD       (*(vu16 *)0x10146000 ^ 0xFFF)
#define BUTTON_SELECT (1 << 2)

#define COLOR_TITLE 0xFF9900
#define COLOR_WHITE 0xFFFFFF

extern int pos_y;

u16 waitInput(void);
void shutdown(u32 mode, char *message);
#pragma once

#include "types.h"

#define HID_PAD       ((~*(vu16 *)0x10146000) & 0xFFF)
#define BUTTON_START  (1 << 3)
#define BUTTON_SELECT (1 << 2)

extern int pos_y;

u16 waitInput(void);
void shutdown(u32 mode, char *message);
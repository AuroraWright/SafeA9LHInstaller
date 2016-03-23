#pragma once

#include "types.h"

#define HID_PAD       ((~*(vu16 *)0x10146000) & 0xFFF)
#define BUTTON_A      (1 << 0)
#define BUTTON_B      (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START  (1 << 3)
#define BUTTON_RIGHT  (1 << 4)
#define BUTTON_LEFT   (1 << 5)
#define BUTTON_UP     (1 << 6)
#define BUTTON_DOWN   (1 << 7)
#define BUTTON_R1     (1 << 8)
#define BUTTON_L1     (1 << 9)
#define BUTTON_X      (1 << 10)
#define BUTTON_Y      (1 << 11)
#define BUTTON_ANY    0x00000FFF

extern int pos_y;

u16 waitInput(void);
void shutdown(u32 mode, char *message);

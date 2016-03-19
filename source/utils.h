#include "types.h"

#pragma once

#define HID_PAD            ((~*(vu16 *)0x10146000) & 0xFFF)

u16 waitInput(void);
void shutdown(u32 mode, int pos_y, char *message);
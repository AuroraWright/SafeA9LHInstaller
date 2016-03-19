#include "types.h"

#pragma once

#define PDN_MPCORE_CFG     (*(vu8 *)0x10140FFC)
#define PDN_SPI_CNT        (*(vu8 *)0x101401C0)
#define BUTTON_SELECT      (1 << 2)
#define BUTTON_START       (1 << 3)

#define OTPOFFSET 0x24000000
#define SECTOROFFSET 0x24100000
#define FIRM0OFFSET 0x24200000
#define FIRM1OFFSET 0x24300000
#define STAGE1OFFSET FIRM0OFFSET + 0xF0400
#define STAGE2OFFSET 0x24400000
#define MAXSTAGE1SIZE 0x1C00
#define MAXSTAGE2SIZE 0x2800
#define TEMPOFFSET 0x24500000

void installer(void);
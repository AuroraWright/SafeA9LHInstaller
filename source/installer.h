/*
*   installer.h
*/

#pragma once

#include "types.h"

#define PDN_MPCORE_CFG     (*(vu8 *)0x10140FFC)
#define PDN_SPI_CNT        (*(vu8 *)0x101401C0)

#define OTP_FROM_MEM    0x10012000
#define OTP_OFFSET      0x24000000
#define SECTOR_OFFSET   0x24100000
#define FIRM0_OFFSET    0x24200000
#define FIRM1_OFFSET    0x24300000
#define FIRM0_SIZE      0xF3000
#define FIRM1_SIZE      0xF2000
#define STAGE1_POSITION 0xF0400
#define STAGE1_OFFSET   FIRM0_OFFSET + STAGE1_POSITION
#define STAGE2_OFFSET   0x24400000
#define MAX_STAGE1_SIZE 0x2000
#define MAX_STAGE2_SIZE 0x2800

static inline void installer(u32 a9lhBoot);
static inline void uninstaller(void);
/*
*   installer.h
*/

#pragma once

#include "types.h"

#define OTP_FROM_MEM      0x10012000
#define FIRM0_OFFSET      0x24000000
#define SECTION2_POSITION 0x66A00
#define FIRM1_OFFSET      0x24100000
#define FIRM0_SIZE        0xF3000
#define FIRM1_SIZE        0xF2000
#define STAGE1_POSITION   0xF0590
#define STAGE1_OFFSET     FIRM0_OFFSET + STAGE1_POSITION
#define STAGE2_OFFSET     0x24200000
#define MAX_STAGE1_SIZE   0x1E70
#define MAX_STAGE2_SIZE   0x89A00

extern const u8 key2s[5][AES_BLOCK_SIZE],
                devKey2s[2][AES_BLOCK_SIZE];

static inline void installer(bool isOtpless);
static inline void uninstaller(void);
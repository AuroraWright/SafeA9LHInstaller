/*
*   installer.c
*/

#include "installer.h"
#include "memory.h"
#include "fs.h"
#include "crypto.h"
#include "screen.h"
#include "draw.h"
#include "utils.h"
#include "types.h"
#include "fatfs/sdmmc/sdmmc.h"
#include "../build/bundled.h"

static const u8 sectorHash[0x20] = {
    0x82, 0xF2, 0x73, 0x0D, 0x2C, 0x2D, 0xA3, 0xF3, 0x01, 0x65, 0xF9, 0x87, 0xFD, 0xCC, 0xAC, 0x5C,
    0xBA, 0xB2, 0x4B, 0x4E, 0x5F, 0x65, 0xC9, 0x81, 0xCD, 0x7B, 0xE6, 0xF4, 0x38, 0xE6, 0xD9, 0xD3
};

static const u8 firm0Hash[0x20] = {
    0x6E, 0x4D, 0x14, 0xAD, 0x51, 0x50, 0xA5, 0x9A, 0x87, 0x59, 0x62, 0xB7, 0x09, 0x0A, 0x3C, 0x74,
    0x4F, 0x72, 0x4B, 0xBD, 0x97, 0x39, 0x33, 0xF2, 0x11, 0xC9, 0x35, 0x22, 0xC8, 0xBB, 0x1C, 0x7D
};

static const u8 firm0A9lhHash[0x20] = {
    0x79, 0x3D, 0x35, 0x7B, 0x8F, 0xF1, 0xFC, 0xF0, 0x8F, 0xB6, 0xDB, 0x51, 0x31, 0xD4, 0xA7, 0x74,
    0x8E, 0xF0, 0x4A, 0xB1, 0xA6, 0x7F, 0xCD, 0xAB, 0x0C, 0x0A, 0xC0, 0x69, 0xA7, 0x9D, 0xC5, 0x04
};

static const u8 firm0100Hash[0x20] = {
    0xD8, 0x2D, 0xB7, 0xB4, 0x38, 0x2B, 0x07, 0x88, 0x99, 0x77, 0x91, 0x0C, 0xC6, 0xEC, 0x6D, 0x87,
    0x7D, 0x21, 0x79, 0x23, 0xD7, 0x60, 0xAF, 0x4E, 0x8B, 0x3A, 0xAB, 0xB2, 0x63, 0xE4, 0x21, 0xC6
};

static const u8 firm1Hash[0x20] = {
    0xD2, 0x53, 0xC1, 0xCC, 0x0A, 0x5F, 0xFA, 0xC6, 0xB3, 0x83, 0xDA, 0xC1, 0x82, 0x7C, 0xFB, 0x3B,
    0x2D, 0x3D, 0x56, 0x6C, 0x6A, 0x1A, 0x8E, 0x52, 0x54, 0xE3, 0x89, 0xC2, 0x95, 0x06, 0x23, 0xE5
};

u32 posY;
bool isN3DS;

void main(void)
{
    //Determine if booting with A9LH
    bool isA9lh = !PDN_SPI_CNT;

    //Detect the console being used
    isN3DS = PDN_MPCORE_CFG == 7;

    vu32 *magic = (vu32 *)0x25000000;
    bool isOtpless = isA9lh && magic[0] == 0xABADCAFE && magic[1] == 0xDEADCAFE;

    sdmmc_sdcard_init();
    initScreens();

    drawString(TITLE, 10, 10, COLOR_TITLE);
    posY = drawString("Thanks to delebile, #cakey and StandardBus", 10, 40, COLOR_WHITE);

    u32 pressed;

    if(!isOtpless)
    {
        posY = drawString(isA9lh ? "Press SELECT to update A9LH, START to uninstall" : "Press SELECT for a full install", 10, posY + SPACING_Y, COLOR_WHITE);
        posY = drawString("Press any other button to shutdown", 10, posY, COLOR_WHITE);
        pressed = waitInput();
    }
    else
    {
        posY = drawString("Finalizing install...", 10, posY + SPACING_Y, COLOR_WHITE);
        pressed = 0;
    }

    if(isOtpless || pressed == BUTTON_SELECT) installer(isA9lh, isOtpless);
    if(pressed == BUTTON_START && isA9lh) uninstaller();

    shutdown(0, NULL);
}

static inline void installer(bool isA9lh, bool isOtpless)
{
    if(!isOtpless && !mountSd())
        shutdown(1, "Error: failed to mount the SD card");

    bool updateA9lh = false;

    //If making a first install on O3DS, we need the OTP
    if(!isA9lh && !isN3DS)
    {
        const char otpPath[] = "a9lh/otp.bin";
        const u8 zeroes[256] = {0};

        //Prefer OTP from memory if available
        if(memcmp((void *)OTP_FROM_MEM, zeroes, 256) == 0)
        {
            // Read OTP from file
            if(fileRead((void *)OTP_OFFSET, otpPath, 256) != 256)
                shutdown(1, "Error: otp.bin doesn't exist and can't be dumped");
        }
        else
        {
            //Write OTP from memory to file
            fileWrite((void *)OTP_FROM_MEM, otpPath, 256);
            memcpy((void *)OTP_OFFSET, (void *)OTP_FROM_MEM, 256);
        }
    }

    //Setup the key sector de/encryption with the SHA register or otp.bin
    if(isA9lh || !isN3DS) setupKeyslot0x11((void *)OTP_OFFSET, isA9lh);

    //Calculate the CTR for the 3DS partitions
    getNandCtr();

    //Get NAND FIRM0 and test that the CTR is correct
    if(!isOtpless)
    {
        readFirm0((u8 *)FIRM0_OFFSET, FIRM0_SIZE);
        if(memcmp((void *)FIRM0_OFFSET, "FIRM", 4) != 0)
            shutdown(1, "Error: failed to setup FIRM encryption");
    }

    //If booting from A9LH or on N3DS, we can use the key sector from NAND
    if(isA9lh || isN3DS) getSector((u8 *)SECTOR_OFFSET, isA9lh);
    else
    {
         //Read decrypted key sector
         if(fileRead((void *)SECTOR_OFFSET, "a9lh/secret_sector.bin", 0x200) != 0x200)
             shutdown(1, "Error: secret_sector.bin doesn't exist or has\na wrong size");
         if(!verifyHash((void *)SECTOR_OFFSET, 0x200, sectorHash))
             shutdown(1, "Error: secret_sector.bin is invalid or corrupted");
    }

    if(isA9lh && !isOtpless)
    {
         u32 i;
         for(i = 0; i < 3; i++)
             if(memcmp((void *)(SECTOR_OFFSET + AES_BLOCK_SIZE), key2s[i], AES_BLOCK_SIZE) == 0) break;

         if(i == 4) shutdown(1, "Error: the OTP hash or the NAND key sector\nare invalid");
         if(i == 0) updateA9lh = true;
    }

    if(!isA9lh)
    {
        //Read FIRM1
        if(fileRead((void *)FIRM1_OFFSET, "a9lh/firm1.bin", FIRM1_SIZE) != FIRM1_SIZE)
            shutdown(1, "Error: firm1.bin doesn't exist or has a wrong size");
        if(!verifyHash((void *)FIRM1_OFFSET, FIRM1_SIZE, firm1Hash))
            shutdown(1, "Error: firm1.bin is invalid or corrupted");

        if(isN3DS)
        {
            //Read FIRM0
            if(fileRead((void *)FIRM0_100_OFFSET, "a9lh/firm0_100.bin", FIRM0100_SIZE) != FIRM0100_SIZE)
                shutdown(1, "Error: firm0_100.bin doesn't exist or has a wrong size");
            if(!verifyHash((void *)FIRM0_100_OFFSET, FIRM0100_SIZE, firm0100Hash))
                shutdown(1, "Error: firm0_100.bin is invalid or corrupted");
        }
    }

    if(!isA9lh || updateA9lh || isOtpless) generateSector((u8 *)SECTOR_OFFSET, (isN3DS && !isA9lh) ? 1 : 0);

    if(!isA9lh || updateA9lh)
    {
        //Read FIRM0
        if(fileRead((void *)FIRM0_OFFSET, "a9lh/firm0.bin", FIRM0_SIZE) != FIRM0_SIZE)
            shutdown(1, "Error: firm0.bin doesn't exist or has a wrong size");
        if(!verifyHash((void *)FIRM0_OFFSET, FIRM0_SIZE, firm0Hash))
            shutdown(1, "Error: firm0.bin is invalid or corrupted");
    }
    else if(!isOtpless && !verifyHash((void *)FIRM0_OFFSET, SECTION2_POSITION, firm0A9lhHash))
        shutdown(1, "Error: NAND FIRM0 is invalid");

    if(!isOtpless)
    {
        //Inject stage1
        memset32((void *)STAGE1_OFFSET, 0, MAX_STAGE1_SIZE);
        if(!fileRead((void *)STAGE1_OFFSET, "a9lh/payload_stage1.bin", MAX_STAGE1_SIZE))
            shutdown(1, "Error: payload_stage1.bin doesn't exist or\nexceeds max size");

        const u8 zeroes[688] = {0};
        if(memcmp(zeroes, (void *)STAGE1_OFFSET, 688) == 0)
            shutdown(1, "Error: the payload_stage1.bin you're attempting\nto install is not compatible");

        //Read stage2
        memset32((void *)STAGE2_OFFSET, 0, MAX_STAGE2_SIZE);
        if(!fileRead((void *)STAGE2_OFFSET, "a9lh/payload_stage2.bin", MAX_STAGE2_SIZE))
            shutdown(1, "Error: payload_stage2.bin doesn't exist or\nexceeds max size");

        posY = drawString("All checks passed, installing...", 10, posY + SPACING_Y, COLOR_WHITE);

        sdmmc_nand_writesectors(0x5C000, MAX_STAGE2_SIZE / 0x200, (u8 *)STAGE2_OFFSET);
    }

    if(!isA9lh) writeFirm((u8 *)FIRM1_OFFSET, true, FIRM1_SIZE);
    if(!isA9lh || updateA9lh || isOtpless) sdmmc_nand_writesectors(0x96, 1, (u8 *)SECTOR_OFFSET);

    if(!isA9lh && isN3DS)
    {
        *(u32 *)0x80FD0FC = 0xEAFFCBBF; //B 0x80F0000
        memcpy((void *)0x80F0000, loader_bin, loader_bin_size);

        writeFirm((u8 *)FIRM0_100_OFFSET, false, FIRM0_SIZE);

        mcuReboot();
    }

    writeFirm((u8 *)FIRM0_OFFSET, false, FIRM0_SIZE);

    shutdown(2, isA9lh && !isOtpless ? "Update: success!" : "Full install: success!");
}

static inline void uninstaller(void)
{
    posY = drawString("You are about to uninstall A9LH!", 10, posY + 10, COLOR_RED);
    posY = drawString("Doing this will require having 9.0 to reinstall!", 10, posY, COLOR_RED);
    posY = drawString("If you would like to continue, press:", 10, posY, COLOR_WHITE);
    posY = drawString("Up, Down, Left, Right, B, A, START, SELECT", 10, posY, COLOR_WHITE);

    u32 unlockSequence[] = { BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT, BUTTON_B, BUTTON_A, BUTTON_START, BUTTON_SELECT },
        sequenceSize = sizeof(unlockSequence) / sizeof(u32);

    for(u32 correctPresses = 0; correctPresses < sequenceSize; correctPresses++)
    {
        if(waitInput() != unlockSequence[correctPresses])
            shutdown(1, "Button sequence not entered correctly");
    }

    //New 3DSes need a key sector with a proper key2, Old 3DSes have a blank key sector
    if(isN3DS)
    {
        setupKeyslot0x11(NULL, true);
        getSector((u8 *)SECTOR_OFFSET, true);
        if(memcmp((void *)(SECTOR_OFFSET + 0x10), key2s[1], 0x10) != 0 && memcmp((void *)(SECTOR_OFFSET + 0x10), key2s[2], 0x10) != 0)
            shutdown(1, "Error: the OTP hash or the NAND key sector\nare invalid");
        generateSector((u8 *)SECTOR_OFFSET, 2);
    }
    else memset32((void *)SECTOR_OFFSET, 0, 0x200);

    if(!mountCtrNand())
        shutdown(1, "Error: failed to mount CTRNAND");

    //Read FIRM cxi from CTRNAND
    switch(firmRead((void *)FIRM0_OFFSET))
    {
        case 1:
            shutdown(1, "Error: more than one FIRM cxi has been detected");
            break;
        case 2:
            shutdown(1, "Error: a FIRM equal or newer than 11.0\nhas been detected");
            break;
        default:
            break;
    }

    //Decrypt it and get its size
    u32 firmSize = decryptExeFs((void *)FIRM0_OFFSET);

    //writeFirm encrypts in-place, so we need two copies
    memcpy((void *)FIRM1_OFFSET, (void *)FIRM0_OFFSET, firmSize);

    //Zero out the stage2 space on NAND
    memset32((void *)STAGE2_OFFSET, 0, MAX_STAGE2_SIZE);

    posY = drawString("All checks passed, uninstalling...", 10, posY + SPACING_Y, COLOR_WHITE);

    //Point of no return, install stuff in the safest order
    sdmmc_nand_writesectors(0x96, 1, (u8 *)SECTOR_OFFSET);
    writeFirm((u8 *)FIRM0_OFFSET, false, firmSize);
    writeFirm((u8 *)FIRM1_OFFSET, true, firmSize);
    sdmmc_nand_writesectors(0x5C000, MAX_STAGE2_SIZE / 0x200, (u8 *)STAGE2_OFFSET);

    shutdown(2, "Uninstall: success!");
}
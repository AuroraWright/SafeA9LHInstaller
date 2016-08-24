/*
*   installer.c
*/

#include "installer.h"
#include "memory.h"
#include "fs.h"
#include "crypto.h"
#include "screeninit.h"
#include "draw.h"
#include "utils.h"
#include "fatfs/sdmmc/sdmmc.h"

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

static const u8 firm1Hash[0x20] = {
    0xD2, 0x53, 0xC1, 0xCC, 0x0A, 0x5F, 0xFA, 0xC6, 0xB3, 0x83, 0xDA, 0xC1, 0x82, 0x7C, 0xFB, 0x3B,
    0x2D, 0x3D, 0x56, 0x6C, 0x6A, 0x1A, 0x8E, 0x52, 0x54, 0xE3, 0x89, 0xC2, 0x95, 0x06, 0x23, 0xE5
};

int posY;

u32 console;

void main(void)
{
    initScreens();
    sdmmc_sdcard_init();

    //Determine if booting with A9LH
    u32 a9lhBoot = !PDN_SPI_CNT;
    //Detect the console being used
    console = PDN_MPCORE_CFG == 7;

    drawString(TITLE, 10, 10, COLOR_TITLE);
    posY = drawString("Thanks to delebile, #cakey and StandardBus", 10, 40, COLOR_WHITE);
    posY = drawString(a9lhBoot ? "Press SELECT to update A9LH, START to uninstall" : "Press SELECT for a full install", 10, posY + SPACING_Y, COLOR_WHITE);
    posY = drawString("Press any other button to shutdown", 10, posY, COLOR_WHITE);

    u32 pressed = waitInput();
    if(pressed == BUTTON_SELECT) installer(a9lhBoot);
    if(pressed == BUTTON_START && a9lhBoot) uninstaller();

    shutdown(0, NULL);
}

static inline void installer(u32 a9lhBoot)
{
    if(!mountSD())
        shutdown(1, "Error: failed to mount the SD card");

    const char *path;
    u32 updatea9lh = 0;

    //If making a first install, we need the OTP
    if(!a9lhBoot)
    {
        // Prefer OTP from memory if available
        const u8 zeroes[256] = {0};
        path = "a9lh/otp.bin";
        if(memcmp((void *)OTP_FROM_MEM, zeroes, 256) == 0)
        {
            // Read OTP from file
            if(fileRead((void *)OTP_OFFSET, path) != 256)
            {
                shutdown(1, "Error: otp.bin doesn't exist and can't be dumped");            
            }
        }
        else
        {
            // Write OTP from memory to file
            fileWrite((void *)OTP_FROM_MEM, path, 256);
            memcpy((void *)OTP_OFFSET, (void *)OTP_FROM_MEM, 256);
        }
    }

    //Setup the key sector de/encryption with the SHA register or otp.bin
    setupKeyslot0x11(a9lhBoot, (void *)OTP_OFFSET);

    //Calculate the CTR for the 3DS partitions
    getNandCTR();

    //Get NAND FIRM0 and test that the CTR is correct
    readFirm0((u8 *)FIRM0_OFFSET, FIRM0_SIZE);
    if(memcmp((void *)FIRM0_OFFSET, "FIRM", 4) != 0)
        shutdown(1, "Error: failed to setup FIRM encryption");

    //If booting from A9LH or on N3DS, we can use the key sector from NAND
    if(a9lhBoot || console)
    {
         getSector((u8 *)SECTOR_OFFSET);

         u32 i;
         for(i = 0; i < 3; i++)
             if(memcmp((void *)(SECTOR_OFFSET + 0x10), key2s[i], 0x10) == 0) break;

         if(i == 3) shutdown(1, a9lhBoot ? "Error: the OTP hash or the NAND key sector\nare invalid" :
                                           "Error: the otp.bin or the NAND key sector\nare invalid");
         else if(i == 1) updatea9lh = 1;
    }
    else
    {
         //Read decrypted key sector
         path = "a9lh/secret_sector.bin";
         if(fileRead((void *)SECTOR_OFFSET, path) != 0x200)
             shutdown(1, "Error: secret_sector.bin doesn't exist or has\na wrong size");
         if(!verifyHash((void *)SECTOR_OFFSET, 0x200, sectorHash))
             shutdown(1, "Error: secret_sector.bin is invalid or corrupted");
    }

    if(!a9lhBoot || updatea9lh)
    {
        //Generate and encrypt a per-console A9LH key sector
        generateSector((u8 *)SECTOR_OFFSET, 0);

        //Read FIRM0
        path = "a9lh/firm0.bin";
        if(fileRead((void *)FIRM0_OFFSET, path) != FIRM0_SIZE)
            shutdown(1, "Error: firm0.bin doesn't exist or has a wrong size");

        if(!verifyHash((void *)FIRM0_OFFSET, FIRM0_SIZE, firm0Hash))
            shutdown(1, "Error: firm0.bin is invalid or corrupted");
    }
    else if(!verifyHash((void *)FIRM0_OFFSET, SECTION2_POSITION, firm0A9lhHash))
        shutdown(1, "Error: NAND FIRM0 is invalid");

    if(!a9lhBoot)
    {
        //Read FIRM1
        path = "a9lh/firm1.bin";
        if(fileRead((void *)FIRM1_OFFSET, path) != FIRM1_SIZE)
            shutdown(1, "Error: firm1.bin doesn't exist or has a wrong size");

        if(!verifyHash((void *)FIRM1_OFFSET, FIRM1_SIZE, firm1Hash))
            shutdown(1, "Error: firm1.bin is invalid or corrupted");
    }

    //Inject stage1
    memset32((void *)STAGE1_OFFSET, 0, MAX_STAGE1_SIZE);
    path = "a9lh/payload_stage1.bin";
    u32 size = fileRead((void *)STAGE1_OFFSET, path);
    if(!size || size > MAX_STAGE1_SIZE)
        shutdown(1, "Error: payload_stage1.bin doesn't exist or\nexceeds max size");

    const u8 zeroes[688] = {0};
    if(memcmp(zeroes, (void *)STAGE1_OFFSET, 688) == 0)
        shutdown(1, "Error: the payload_stage1.bin you're attempting\nto install is not compatible");

    //Read stage2
    memset32((void *)STAGE2_OFFSET, 0, MAX_STAGE2_SIZE);
    path = "a9lh/payload_stage2.bin";
    size = fileRead((void *)STAGE2_OFFSET, path);
    if(!size || size > MAX_STAGE2_SIZE)
        shutdown(1, "Error: payload_stage2.bin doesn't exist or\nexceeds max size");

    posY = drawString("All checks passed, installing...", 10, posY + SPACING_Y, COLOR_WHITE);

    //Point of no return, install stuff in the safest order
    sdmmc_nand_writesectors(0x5C000, MAX_STAGE2_SIZE / 0x200, (vu8 *)STAGE2_OFFSET);
    if(!a9lhBoot) writeFirm((u8 *)FIRM1_OFFSET, 1, FIRM1_SIZE);
    if(!a9lhBoot || updatea9lh) sdmmc_nand_writesectors(0x96, 1, (vu8 *)SECTOR_OFFSET);
    writeFirm((u8 *)FIRM0_OFFSET, 0, FIRM0_SIZE);

    shutdown(2, a9lhBoot ? "Update: success!" : "Full install: success!");
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
    if(console)
    {
        setupKeyslot0x11(1, NULL);
        getSector((u8 *)SECTOR_OFFSET);
        if(memcmp((void *)(SECTOR_OFFSET + 0x10), key2s[1], 0x10) != 0 && memcmp((void *)(SECTOR_OFFSET + 0x10), key2s[2], 0x10) != 0)
            shutdown(1, "Error: the OTP hash or the NAND key sector\nare invalid");
        generateSector((u8 *)SECTOR_OFFSET, 1);
    }
    else memset32((void *)SECTOR_OFFSET, 0, 0x200);

    if(!mountCTRNAND())
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
    sdmmc_nand_writesectors(0x96, 1, (vu8 *)SECTOR_OFFSET);
    writeFirm((u8 *)FIRM0_OFFSET, 0, firmSize);
    writeFirm((u8 *)FIRM1_OFFSET, 1, firmSize);
    sdmmc_nand_writesectors(0x5C000, MAX_STAGE2_SIZE / 0x200, (vu8 *)STAGE2_OFFSET);

    shutdown(2, "Uninstall: success!");
}

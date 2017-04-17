/*
*   installer.c
*/

#include "memory.h"
#include "fs.h"
#include "crypto.h"
#include "screen.h"
#include "draw.h"
#include "utils.h"
#include "types.h"
#include "installer.h"
#include "fatfs/sdmmc/sdmmc.h"

static const u8 sectorHashRetail[SHA_256_HASH_SIZE] = {
    0x82, 0xF2, 0x73, 0x0D, 0x2C, 0x2D, 0xA3, 0xF3, 0x01, 0x65, 0xF9, 0x87, 0xFD, 0xCC, 0xAC, 0x5C,
    0xBA, 0xB2, 0x4B, 0x4E, 0x5F, 0x65, 0xC9, 0x81, 0xCD, 0x7B, 0xE6, 0xF4, 0x38, 0xE6, 0xD9, 0xD3
},
                firm0HashRetail[SHA_256_HASH_SIZE] = {
    0x6E, 0x4D, 0x14, 0xAD, 0x51, 0x50, 0xA5, 0x9A, 0x87, 0x59, 0x62, 0xB7, 0x09, 0x0A, 0x3C, 0x74,
    0x4F, 0x72, 0x4B, 0xBD, 0x97, 0x39, 0x33, 0xF2, 0x11, 0xC9, 0x35, 0x22, 0xC8, 0xBB, 0x1C, 0x7D
},
                firm0A9lhHashRetail[SHA_256_HASH_SIZE] = {
    0x79, 0x3D, 0x35, 0x7B, 0x8F, 0xF1, 0xFC, 0xF0, 0x8F, 0xB6, 0xDB, 0x51, 0x31, 0xD4, 0xA7, 0x74,
    0x8E, 0xF0, 0x4A, 0xB1, 0xA6, 0x7F, 0xCD, 0xAB, 0x0C, 0x0A, 0xC0, 0x69, 0xA7, 0x9D, 0xC5, 0x04
},
                firm090A9lhHash[SHA_256_HASH_SIZE] = {
    0x68, 0x52, 0xCC, 0x21, 0x89, 0xAE, 0x28, 0x38, 0x1A, 0x75, 0x90, 0xE7, 0x38, 0x23, 0x48, 0x41,
    0x8E, 0x80, 0x78, 0x75, 0x27, 0x64, 0x04, 0xD6, 0x28, 0xD6, 0xFA, 0x39, 0xA8, 0x6F, 0xB0, 0x3F
},
                firm1HashRetail[SHA_256_HASH_SIZE] = {
    0xD8, 0x2D, 0xB7, 0xB4, 0x38, 0x2B, 0x07, 0x88, 0x99, 0x77, 0x91, 0x0C, 0xC6, 0xEC, 0x6D, 0x87,
    0x7D, 0x21, 0x79, 0x23, 0xD7, 0x60, 0xAF, 0x4E, 0x8B, 0x3A, 0xAB, 0xB2, 0x63, 0xE4, 0x21, 0xC6
},
                sectorHashDev[SHA_256_HASH_SIZE] = {
    0xB2, 0x91, 0xD9, 0xB1, 0x33, 0x05, 0x79, 0x0D, 0x47, 0xC6, 0x06, 0x98, 0x4C, 0x67, 0xC3, 0x70, 
    0x09, 0x54, 0xE3, 0x85, 0xDE, 0x47, 0x55, 0xAF, 0xC6, 0xCB, 0x1D, 0x8D, 0xC7, 0x84, 0x5A, 0x64
},
                firm0HashDev[SHA_256_HASH_SIZE] = {
    0xCD, 0x62, 0xA6, 0x58, 0x40, 0x1B, 0x8B, 0x8F, 0xD3, 0x2C, 0x72, 0x58, 0xD8, 0x24, 0x21, 0x36,
    0xCF, 0x83, 0x40, 0xA3, 0x34, 0x8E, 0xED, 0x33, 0x0A, 0x1A, 0x16, 0x04, 0x49, 0xC9, 0x74, 0x3E
},
                firm0A9lhHashDev[SHA_256_HASH_SIZE] = {
    0x60, 0xBB, 0xD1, 0x35, 0x44, 0x2F, 0xBD, 0x47, 0x69, 0xBF, 0x36, 0x4B, 0x0B, 0x79, 0x6E, 0x4C,
    0xE1, 0xB2, 0xDB, 0x7A, 0xAD, 0xF0, 0x04, 0x31, 0xCB, 0xBD, 0x54, 0xD3, 0x99, 0x8C, 0x9C, 0xD2
},
                firm1HashDev[SHA_256_HASH_SIZE] = {
    0xCD, 0x87, 0x85, 0x33, 0x76, 0xCA, 0x2A, 0x3F, 0xFC, 0x24, 0x4C, 0x29, 0x95, 0x8B, 0xA8, 0x34,
    0xF2, 0x38, 0x14, 0x58, 0x10, 0x83, 0x56, 0x4F, 0x0D, 0x5A, 0xDB, 0x29, 0x12, 0xD8, 0xA9, 0x84
};

u32 posY;

static void drawTitle(void)
{
    initScreens();

    posY = drawString(TITLE, 10, 10, COLOR_TITLE);
    posY = drawString("Thanks to delebile, #cakey and StandardBus", 10, posY + SPACING_Y, COLOR_WHITE);
}

void main(void)
{
    bool isOtpless = ISA9LH && *(vu32 *)0x80FD0FC == 0xEAFE4AA3 && magic == 0xDEADCAFE;

    if(!isOtpless) drawTitle();

    if(!sdmmc_sdcard_init(!isOtpless, true) && !isOtpless)
        shutdown(1, "Error: failed to initialize SD and NAND");

    u32 pressed;

    if(!isOtpless)
    {
        posY = drawString(ISA9LH ? "Press SELECT to update A9LH, START to uninstall" : "Press SELECT for a full install", 10, posY + SPACING_Y, COLOR_WHITE);
        posY = drawString("Press any other button to shutdown", 10, posY, COLOR_WHITE);
        pressed = waitInput();
    }

    if(isOtpless || pressed == BUTTON_SELECT) installer(isOtpless);
    if(pressed == BUTTON_START && ISA9LH) uninstaller();

    shutdown(0, NULL);
}

static inline void installer(bool isOtpless)
{
    bool updateKey2 = false,
         updateFirm0 = false,
         updateFirm1 = false;
    u8 otp[256] = {0},
       keySector[512];

    if(!isOtpless && !mountFs(true))
        shutdown(1, "Error: failed to mount the SD card");

    //If making a first install on O3DS, we need the OTP
    if(!ISA9LH && (!ISN3DS || ISDEVUNIT))
    {
        const char otpPath[] = "a9lh/otp.bin";

        //Prefer OTP from memory if available
        if(memcmp((void *)OTP_FROM_MEM, otp, sizeof(otp)) == 0)
        {
            // Read OTP from file
            if(fileRead(otp, otpPath, sizeof(otp)) != sizeof(otp))
                shutdown(1, "Error: otp.bin doesn't exist and can't be dumped");
        }
        else
        {
            //Write OTP from memory to file
            fileWrite((void *)OTP_FROM_MEM, otpPath, sizeof(otp));
            memcpy(otp, (void *)OTP_FROM_MEM, sizeof(otp));
        }
    }

    //Setup the key sector de/encryption with the SHA register or otp.bin
    if(ISA9LH || !ISN3DS || ISDEVUNIT) setupKeyslot0x11(otp);

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
    if(ISA9LH || ISN3DS) getSector(keySector);
    else
    {
         //Read decrypted key sector
         if(fileRead(keySector, "a9lh/secret_sector.bin", sizeof(keySector)) != sizeof(keySector))
             shutdown(1, "Error: secret_sector.bin doesn't exist or has\na wrong size");
         if(!verifyHash(keySector, sizeof(keySector), !ISDEVUNIT ? sectorHashRetail : sectorHashDev))
             shutdown(1, "Error: secret_sector.bin is invalid or corrupted");
    }

    if(ISA9LH && !isOtpless)
    {
        u32 i;

        if(!ISDEVUNIT)
        {
            for(i = 1; i < 5; i++)
                if(memcmp(keySector + AES_BLOCK_SIZE, key2s[i], AES_BLOCK_SIZE) == 0) break;
        }
        else i = memcmp(keySector + AES_BLOCK_SIZE, devKey2s[1], AES_BLOCK_SIZE) == 0 ? 1 : 5;

        switch(i)
        {
            case 5:
                shutdown(1, "Error: the OTP hash or the NAND key sector\nare invalid");
                break;
            case 2:
            case 3:
            case 4:
                updateFirm1 = true;
                updateKey2 = true;
                break;
            case 1:
                break;
        }

        if(!verifyHash((void *)FIRM0_OFFSET, SECTION2_POSITION, !ISDEVUNIT ? firm0A9lhHashRetail : firm0A9lhHashDev))
        {
            if(ISDEVUNIT || !verifyHash((void *)FIRM0_OFFSET, SECTION2_POSITION, firm090A9lhHash))
                shutdown(1, "Error: NAND FIRM0 is invalid");

            updateFirm0 = true;
        }
    }

    if(!ISA9LH || updateKey2 || isOtpless) generateSector(keySector, (!ISA9LH && ISN3DS && !ISDEVUNIT) ? 1 : 0);

    if(!ISA9LH || updateFirm0)
    {
        //Read FIRM0
        if(fileRead((void *)FIRM0_OFFSET, "a9lh/firm0.bin", FIRM0_SIZE) != FIRM0_SIZE)
            shutdown(1, "Error: firm0.bin doesn't exist or has a wrong size");
        if(!verifyHash((void *)FIRM0_OFFSET, FIRM0_SIZE, !ISDEVUNIT ? firm0HashRetail : firm0HashDev))
            shutdown(1, "Error: firm0.bin is invalid or corrupted");
    }

    if(!ISA9LH || updateFirm1)
    {
        //Read FIRM1
        if(fileRead((void *)FIRM1_OFFSET, "a9lh/firm1.bin", FIRM1_SIZE) != FIRM1_SIZE)
            shutdown(1, "Error: firm1.bin doesn't exist or has a wrong size");
        if(!verifyHash((void *)FIRM1_OFFSET, FIRM1_SIZE, !ISDEVUNIT ? firm1HashRetail : firm1HashDev))
            shutdown(1, "Error: firm1.bin is invalid or corrupted");
    }

    if(!ISA9LH && ISN3DS && !ISDEVUNIT)
    {
        magic = 0xDEADCAFE;

        fileRename("arm9loaderhax.bin", "arm9loaderhax.bak");

        if(!fileWrite((void *)0x23F00000, "arm9loaderhax.bin", 0x10000))
            shutdown(1, "Error: couldn't write arm9loaderhax.bin");
    }

    if(!isOtpless)
    {
        bool missingStage1Hash,
             missingStage2Hash;
        u8 stageHash[SHA_256_HASH_SIZE];

        //Inject stage1
        memset32((void *)STAGE1_OFFSET, 0, MAX_STAGE1_SIZE);
        u32 stageSize = fileRead((void *)STAGE1_OFFSET, "a9lh/payload_stage1.bin", MAX_STAGE1_SIZE);
        if(!stageSize)
            shutdown(1, "Error: payload_stage1.bin doesn't exist or\nexceeds max size");

        const u8 zeroes[688] = {0};
        if(memcmp(zeroes, (void *)STAGE1_OFFSET, 688) == 0)
            shutdown(1, "Error: the payload_stage1.bin you're attempting\nto install is not compatible");

        //Verify stage1
        if(fileRead(stageHash, "a9lh/payload_stage1.bin.sha", sizeof(stageHash)) == sizeof(stageHash))
        {
            if(!verifyHash((void *)STAGE1_OFFSET, stageSize, stageHash))
                shutdown(1, "Error: payload_stage1.bin is invalid\nor corrupted");

            missingStage1Hash = false;
        }
        else missingStage1Hash = true;

        //Read stage2
        memset32((void *)STAGE2_OFFSET, 0, MAX_STAGE2_SIZE);
        stageSize = fileRead((void *)STAGE2_OFFSET, "a9lh/payload_stage2.bin", MAX_STAGE2_SIZE);
        if(!stageSize)
            shutdown(1, "Error: payload_stage2.bin doesn't exist or\nexceeds max size");

        //Verify stage2
        if(fileRead(stageHash, "a9lh/payload_stage2.bin.sha", sizeof(stageHash)) == sizeof(stageHash))
        {
            if(!verifyHash((void *)STAGE2_OFFSET, stageSize, stageHash))
                shutdown(1, "Error: payload_stage2.bin is invalid\nor corrupted");

            missingStage2Hash = false;
        }
        else missingStage2Hash = true;

        if(missingStage1Hash || missingStage2Hash)
        {
            posY = drawString("Couldn't verify stage1 and/or stage2 integrity!", 10, posY + SPACING_Y, COLOR_RED);
            posY = drawString("Continuing might be dangerous!", 10, posY, COLOR_RED);
            inputSequence();
        }

        posY = drawString("All checks passed, installing...", 10, posY + SPACING_Y, COLOR_WHITE);

        sdmmc_nand_writesectors(0x5C000, MAX_STAGE2_SIZE / 0x200, (u8 *)STAGE2_OFFSET);
    }

    if(!ISA9LH || updateFirm1) writeFirm((u8 *)FIRM1_OFFSET, true, FIRM1_SIZE);
    if(!ISA9LH || updateKey2 || isOtpless) sdmmc_nand_writesectors(0x96, 1, keySector);
    if(!isOtpless) writeFirm((u8 *)FIRM0_OFFSET, false, FIRM0_SIZE);
    else
    {
        *(vu32 *)0x80FD0FC = 0;

        drawTitle();

        if(sdmmc_sdcard_init(true, false) && mountFs(true))
        {
            fileDelete("arm9loaderhax.bin");
            fileRename("arm9loaderhax.bak", "arm9loaderhax.bin");
        }
        else
        {
            posY = drawString("Couldn't remove arm9loaderhax.bin!", 10, posY + SPACING_Y, COLOR_RED);
            posY = drawString("Do it yourself after the install ends", 10, posY, COLOR_RED);
        }
    }

    if(!ISA9LH && ISN3DS && !ISDEVUNIT)
    {
        *(vu32 *)0x80FD0FC = 0xEAFE4AA3;

        mcuReboot();
    }

    shutdown(2, ISA9LH && !isOtpless ? "Update: success!" : "Full install: success!");
}

static inline void uninstaller(void)
{
    u8 keySector[512];

    //New 3DSes need a key sector with a proper key2, Old 3DSes have a blank key sector
    if(ISN3DS)
    {
        setupKeyslot0x11(NULL);
        getSector(keySector);

        u32 i;

        if(!ISDEVUNIT)
        {
            for(i = 1; i < 5; i++)
                if(memcmp(keySector + AES_BLOCK_SIZE, key2s[i], AES_BLOCK_SIZE) == 0) break;
        }
        else i = memcmp(keySector + AES_BLOCK_SIZE, devKey2s[1], AES_BLOCK_SIZE) == 0 ? 1 : 5;

        if(i == 5) shutdown(1, "Error: the OTP hash or the NAND key sector\nare invalid");

        generateSector(keySector, 2);
    }
    else memset32(keySector, 0, sizeof(keySector));

    if(!mountFs(false))
        shutdown(1, "Error: failed to mount CTRNAND");

    //Read FIRM cxi from CTRNAND
    u32 firmSize = 0,
        result = firmRead((void *)FIRM0_OFFSET);

    switch(result)
    {
        case 1:
            shutdown(1, "Error: more than one FIRM has been detected");
            break;
        case 2:
            posY = drawString("A FIRM newer than 11.3 has been detected!", 10, posY + SPACING_Y, COLOR_RED);
            posY = drawString("You are about to uninstall A9LH!", 10, posY + SPACING_Y, COLOR_RED);
            posY = drawString("To reinstall you'll need hardmod + NAND backup!", 10, posY, COLOR_RED);
            break;
        case 3:
            shutdown(1, "Error: the CTRNAND FIRM is too large");
            break;
        case 4:
            shutdown(1, "Error: couldn't read FIRM from CTRNAND");
            break;
        default:
            posY = drawString("You are about to uninstall A9LH!", 10, posY + SPACING_Y, COLOR_RED);
            posY = drawString("To reinstall you'll need 9.2 or lower!", 10, posY, COLOR_RED);
            break;
    }

    inputSequence();

    //Decrypt it and get its size
    firmSize = decryptExeFs((Cxi *)FIRM0_OFFSET);
    if(firmSize == 0) shutdown(1, "Error: couldn't decrypt the CTRNAND FIRM");

    //writeFirm encrypts in-place, so we need two copies
    memcpy((void *)FIRM1_OFFSET, (void *)FIRM0_OFFSET, firmSize);

    //Zero out the stage2 space on NAND
    memset32((void *)STAGE2_OFFSET, 0, MAX_STAGE2_SIZE);

    posY = drawString("All checks passed, uninstalling...", 10, posY + SPACING_Y, COLOR_WHITE);

    //Point of no return, install stuff in the safest order
    sdmmc_nand_writesectors(0x96, 1, keySector);
    writeFirm((u8 *)FIRM0_OFFSET, false, firmSize);
    writeFirm((u8 *)FIRM1_OFFSET, true, firmSize);
    sdmmc_nand_writesectors(0x5C000, MAX_STAGE2_SIZE / 0x200, (u8 *)STAGE2_OFFSET);

    shutdown(2, "Uninstall: success!");
}
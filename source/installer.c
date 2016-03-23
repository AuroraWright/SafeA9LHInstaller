#include "installer.h"
#include "memory.h"
#include "fs.h"
#include "crypto.h"
#include "draw.h"
#include "utils.h"
#include "fatfs/sdmmc/sdmmc.h"

static const u8 sectorHash[0x20] = {
    0x82, 0xF2, 0x73, 0x0D, 0x2C, 0x2D, 0xA3, 0xF3, 0x01, 0x65, 0xF9, 0x87, 0xFD, 0xCC, 0xAC, 0x5C,
    0xBA, 0xB2, 0x4B, 0x4E, 0x5F, 0x65, 0xC9, 0x81, 0xCD, 0x7B, 0xE6, 0xF4, 0x38, 0xE6, 0xD9, 0xD3
};

static const u8 firm0Hash[0x20] = {
    0xD7, 0xBE, 0x76, 0xE1, 0x81, 0x3F, 0x39, 0x8D, 0xCE, 0xA8, 0x55, 0x72, 0xD0, 0xC0, 0x58, 0xF7,
    0x95, 0x47, 0x61, 0xA1, 0xD5, 0xEA, 0x03, 0xB5, 0xEB, 0x50, 0x47, 0xAC, 0x63, 0xAC, 0x5D, 0x6B
};

static const u8 firm1Hash[0x20] = {
    0xD2, 0x53, 0xC1, 0xCC, 0x0A, 0x5F, 0xFA, 0xC6, 0xB3, 0x83, 0xDA, 0xC1, 0x82, 0x7C, 0xFB, 0x3B,
    0x2D, 0x3D, 0x56, 0x6C, 0x6A, 0x1A, 0x8E, 0x52, 0x54, 0xE3, 0x89, 0xC2, 0x95, 0x06, 0x23, 0xE5
};

static const char *path;
static u32 size;
int pos_y;

static void installStage2(u32 mode){
    //Warning sequence
    u32 pad_state;
    u32 unlockSequenceState2[] = { BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT, BUTTON_B, BUTTON_A, BUTTON_START, BUTTON_SELECT };
    u32 unlockLvlMax = sizeof(unlockSequenceState2) / sizeof(u32);
    u32* unlockSequence = unlockSequenceState2;
    u32 unlockLvl = 0;
    pos_y = drawString("You are about to update stage2 only.", 10, pos_y + 10, 0x0000FF);
    pos_y = drawString("Doing this could brick your console!", 10, pos_y, 0x0000FF);
    pos_y = drawString("If you would like to continue, enter:", 10, pos_y + 10, 0xffffff);
    pos_y = drawString("<Up>, <Down>, <Left>, <Right>,", 10, pos_y, 0xffffff);
    pos_y = drawString("<B>, <A>, <START> <SELECT>", 10, pos_y, 0xffffff);
    while (true) {
        if (unlockLvl == unlockLvlMax) {
            break;
        }
        pad_state = (u32) waitInput();
        if (!(pad_state & BUTTON_ANY))
            continue;
        else if (pad_state & unlockSequence[unlockLvl])
            unlockLvl++;
        else if (unlockLvl == 0 || !(pad_state & unlockSequence[unlockLvl-1]))
            shutdown(1, "Sequence entered incorrectly.");

    }
    
    //Read stage2
    pos_y = drawString("Installing stage2...", 10, pos_y, 0xffffff);
    path = "a9lh/payload_stage2.bin";
    size = fileSize(path);
    if(!size || size > MAXSTAGE2SIZE)
        shutdown(1, "Error: stage2.bin doesn't exist or exceeds\nmax size");
    memset((u8 *)STAGE2OFFSET, 0, MAXSTAGE2SIZE);
    fileRead((u8 *)STAGE2OFFSET, path, size);
    if(mode) return;
    sdmmc_nand_writesectors(0x5C000, 0x20, (vu8 *)STAGE2OFFSET);
    shutdown(2, "Stage2 update: success!");
    
}

void installer(void){
    //Determine if booting with A9LH
    u32 a9lhBoot = (PDN_SPI_CNT == 0x0) ? 1 : 0;
    //Detect the console being used
    u32 console = (PDN_MPCORE_CFG == 1) ? 0 : 1;

    while(1){
        clearScreens();
        drawString("Safe A9LH Installer v1.4", 10, 10, 0xff9900);
        pos_y = drawString("Thanks to delebile, #cakey and StandardBus", 10, 40, 0xFFFFFF);
        pos_y = drawString("Press SELECT for a full install", 10, pos_y + SPACING_VERT, 0xFFFFFF);
        if(a9lhBoot){ pos_y = drawString("Press START to only update stage2", 10, pos_y, 0xFFFFFF);
        pos_y = drawString("(Only do stage2 updates from the fork you use!)", 10, pos_y, 0x0000FF); }
        pos_y = drawString("Press any other button to shutdown", 10, pos_y, 0xFFFFFF);

        u16 pressed = waitInput();
        if(a9lhBoot && pressed == BUTTON_START) installStage2(0);
        else if(pressed == BUTTON_SELECT) break;
        else shutdown(0, NULL);
    }

    //If making a first install, we need the OTP
    if(!a9lhBoot){
        //Read OTP
        path = "a9lh/otp.bin";
        if(fileSize(path) != 256)
            shutdown(1, "Error: otp.bin doesn't exist or has a wrong size");
        fileRead((u8 *)OTPOFFSET, path, 256);
    }

    //Setup the key sector de/encryption with the SHA register or otp.bin
    setupKeyslot0x11(a9lhBoot, (u8 *)OTPOFFSET);

    if(a9lhBoot && !testOtp(a9lhBoot))
        shutdown(1, "Error: the OTP hash is invalid");

    if(!a9lhBoot && console && !testOtp(a9lhBoot))
        shutdown(1, "Error: otp.bin is invalid or corrupted");

    //Calculate the CTR for the 3DS partitions
    getNandCTR();

    //Test that the CTR is correct
    readFirm0((u8 *)TEMPOFFSET, 0x200);
    if(memcmp((void *)TEMPOFFSET, "FIRM", 4) != 0)
        shutdown(1, "Error: failed to setup FIRM encryption");

    //Read decrypted key sector
    path = "a9lh/secret_sector.bin";
    if(fileSize(path) != 0x200)
        shutdown(1, "Error: secret_sector.bin doesn't exist or has\na wrong size");
    fileRead((u8 *)SECTOROFFSET, path, 0x200);
    if(!verifyHash((u8 *)SECTOROFFSET, 0x200, sectorHash))
        shutdown(1, "Error: secret_sector.bin is invalid or corrupted");

    //Generate and encrypt a per-console A9LH key sector
    generateSector((void *)SECTOROFFSET);

    //Read FIRM0
    path = "a9lh/firm0.bin";
    u32 firm0Size = fileSize(path);
    if(!firm0Size)
        shutdown(1, "Error: firm0.bin doesn't exist");
    fileRead((u8 *)FIRM0OFFSET, path, firm0Size);
    if(!verifyHash((u8 *)FIRM0OFFSET, firm0Size, firm0Hash))
        shutdown(1, "Error: firm0.bin is invalid or corrupted");

    //Read FIRM1
    path = "a9lh/firm1.bin";
    u32 firm1Size = fileSize(path);
    if(!firm1Size)
        shutdown(1, "Error: firm1.bin doesn't exist");
    fileRead((u8 *)FIRM1OFFSET, path, firm1Size);
    if(!verifyHash((u8 *)FIRM1OFFSET, firm1Size, firm1Hash))
        shutdown(1, "Error: firm1.bin is invalid or corrupted");

    //Inject stage1
    path = "a9lh/payload_stage1.bin";
    size = fileSize(path);
    if(!size || size > MAXSTAGE1SIZE)
        shutdown(1, "Error: stage1.bin doesn't exist or exceeds\nmax size");
    fileRead((u8 *)STAGE1OFFSET, path, size);

    installStage2(1);

    pos_y = drawString("All checks passed, installing...", 10, pos_y + SPACING_VERT, 0xFFFFFF);

    //Point of no return, install stuff in the safest order
    sdmmc_nand_writesectors(0x5C000, 0x20, (vu8 *)STAGE2OFFSET);
    writeFirm((u8 *)FIRM1OFFSET, 1, firm1Size);
    writeFirm((u8 *)FIRM0OFFSET, 0, firm0Size);
    sdmmc_nand_writesectors(0x96, 0x1, (vu8 *)SECTOROFFSET);

    shutdown(1, "Full install: success!");
}
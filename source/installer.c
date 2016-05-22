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
    0x6E, 0x4D, 0x14, 0xAD, 0x51, 0x50, 0xA5, 0x9A, 0x87, 0x59, 0x62, 0xB7, 0x09, 0x0A, 0x3C, 0x74,
    0x4F, 0x72, 0x4B, 0xBD, 0x97, 0x39, 0x33, 0xF2, 0x11, 0xC9, 0x35, 0x22, 0xC8, 0xBB, 0x1C, 0x7D
};

static const u8 firm1Hash[0x20] = {
    0x43, 0x17, 0x03, 0x39, 0x89, 0xCD, 0xE1, 0xE1, 0x7B, 0x89, 0x7B, 0x32, 0x71, 0x07, 0x0C, 0xE9,
    0x20, 0xBF, 0x4A, 0x95, 0x2C, 0x69, 0xDE, 0xB8, 0x72, 0x1C, 0xA1, 0x3B, 0x52, 0x68, 0x23, 0xC1
};

int pos_y;

void checkSector(void) {
    char* path;
    path = "a9lh/secret_sector.bin";
    if(fileSize(path) != 0x200)
        shutdown(1, "Error: secret_sector.bin doesn't exist or has\na wrong size");
    fileRead((void *)SECTOR_OFFSET, path, 0x200);
    if(!verifyHash((void *)SECTOR_OFFSET, 0x200, sectorHash))
        shutdown(1, "Error: secret_sector.bin is invalid or corrupted");
}

void installer(void){
    //Determine if booting with A9LH
    u32 a9lhBoot = (PDN_SPI_CNT == 0x0) ? 1 : 0;
    //Detect the console being used
    u32 console = (PDN_MPCORE_CFG == 1) ? 0 : 1;
    
    drawString(TITLE, 10, 10, COLOR_TITLE);
    pos_y = drawString("Thanks to delebile, #cakey and StandardBus", 10, 40, COLOR_WHITE);
    pos_y = drawString(a9lhBoot ? "Press SELECT to update A9LH" : "Press SELECT for a full install", 10, pos_y + SPACING_VERT, COLOR_WHITE);
    pos_y = drawString("Press any other button to shutdown", 10, pos_y, COLOR_WHITE);
    
    if(waitInput() != BUTTON_SELECT) shutdown(0, NULL);
    
    const char *path;
    
    //If making a first install, we need the OTP
    if(!a9lhBoot){
        //Read OTP
        path = "a9lh/otp.bin";
        if(fileSize(path) != 256)
        shutdown(1, "Error: otp.bin doesn't exist or has a wrong size");
        fileRead((void *)OTP_OFFSET, path, 256);
    }
    
    //Setup the key sector de/encryption with the SHA register or otp.bin
    setupKeyslot0x11(a9lhBoot, (void *)OTP_OFFSET);
    
    //Calculate the CTR for the 3DS partitions
    getNandCTR();
    
    //Get NAND FIRM0 and test that the CTR is correct
    readFirm0((u8 *)FIRM0_OFFSET, FIRM_SIZE);
    if(memcmp((void *)FIRM0_OFFSET, "FIRM", 4) != 0)
    shutdown(1, "Error: failed to setup FIRM encryption");
    

    //Read decrypted key sector
    path = "a9lh/secret_sector.bin";
    if(fileSize(path) != 0x200)
    shutdown(1, "Error: secret_sector.bin doesn't exist or has\na wrong size");
    fileRead((void *)SECTOR_OFFSET, path, 0x200);
    if(!verifyHash((void *)SECTOR_OFFSET, 0x200, sectorHash))
    shutdown(1, "Error: secret_sector.bin is invalid or corrupted");
    
    

    //Generate and encrypt a per-console A9LH key sector
    generateSector((u8 *)SECTOR_OFFSET);
        
    //Read FIRM0
    path = "a9lh/firm0.bin";
    if(fileSize(path) != FIRM_SIZE)
    shutdown(1, "Error: firm0.bin doesn't exist or has a wrong size");
    fileRead((void *)FIRM0_OFFSET, path, FIRM_SIZE);
    if(!verifyHash((void *)FIRM0_OFFSET, FIRM_SIZE, firm0Hash))
    shutdown(1, "Error: firm0.bin is invalid or corrupted");
        
    //Read FIRM1
    path = "a9lh/firm1.bin";
    if(fileSize(path) != FIRM_SIZE)
    shutdown(1, "Error: firm1.bin doesn't exist or has a wrong size");
    fileRead((void *)FIRM1_OFFSET, path, FIRM_SIZE);
    if(!verifyHash((void *)FIRM1_OFFSET, FIRM_SIZE, firm1Hash))
    shutdown(1, "Error: firm1.bin is invalid or corrupted");


    
    //Inject stage1
    path = "a9lh/payload_stage1.bin";
    u32 size = fileSize(path);
    if(!size || size > MAX_STAGE1_SIZE)
    shutdown(1, "Error: payload_stage1.bin doesn't exist or\nexceeds max size");
    memset((void *)STAGE1_OFFSET, 0, MAX_STAGE1_SIZE);
    fileRead((void *)STAGE1_OFFSET, path, size);
    
    //Read stage2
    path = "a9lh/payload_stage2.bin";
    size = fileSize(path);
    if(!size || size > MAX_STAGE2_SIZE)
    shutdown(1, "Error: payload_stage2.bin doesn't exist or\nexceeds max size");
    memset((void *)STAGE2_OFFSET, 0, MAX_STAGE2_SIZE);
    fileRead((void *)STAGE2_OFFSET, path, size);
    
    pos_y = drawString("All checks passed, installing...", 10, pos_y + SPACING_VERT, COLOR_WHITE);
    
    //Point of no return, install stuff in the safest order
    sdmmc_nand_writesectors(0x5C000, 0x20, (vu8 *)STAGE2_OFFSET);
    if(!a9lhBoot){ writeFirm((u8 *)FIRM1_OFFSET, 1, FIRM_SIZE);
        sdmmc_nand_writesectors(0x96, 1, (vu8 *)SECTOR_OFFSET); }
    writeFirm((u8 *)FIRM0_OFFSET, 0, FIRM_SIZE);
    
    shutdown(2, a9lhBoot ? "Update: success!" : "Full install: success!");
}

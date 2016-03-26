// From http://github.com/b1l1s/ctr

#pragma once

#include "types.h"

/**************************AES****************************/
#define REG_AESCNT		((vu32 *)0x10009000)
#define REG_AESBLKCNT		((vu32 *)0x10009004)
#define REG_AESWRFIFO		((vu32 *)0x10009008)
#define REG_AESRDFIFO		((vu32 *)0x1000900C)
#define REG_AESKEYSEL		((vu8 *)0x10009010)
#define REG_AESKEYCNT		((vu8 *)0x10009011)
#define REG_AESCTR		((vu32 *)0x10009020)

#define REG_AESKEYFIFO		((vu32 *)0x10009100)
#define REG_AESKEYXFIFO		((vu32 *)0x10009104)
#define REG_AESKEYYFIFO		((vu32 *)0x10009108)

#define AES_CCM_DECRYPT_MODE	(0u << 27)
#define AES_CCM_ENCRYPT_MODE	(1u << 27)
#define AES_CTR_MODE		(2u << 27)
#define AES_CTR_MODE		(2u << 27)
#define AES_CBC_DECRYPT_MODE	(4u << 27)
#define AES_CBC_ENCRYPT_MODE	(5u << 27)
#define AES_ECB_DECRYPT_MODE	(6u << 27)
#define AES_ECB_ENCRYPT_MODE	(7u << 27)
#define AES_ALL_MODES		(7u << 27)

#define AES_CNT_START		0x80000000
#define AES_CNT_INPUT_ORDER	0x02000000
#define AES_CNT_OUTPUT_ORDER	0x01000000
#define AES_CNT_INPUT_ENDIAN	0x00800000
#define AES_CNT_OUTPUT_ENDIAN	0x00400000
#define AES_CNT_FLUSH_READ	0x00000800
#define AES_CNT_FLUSH_WRITE	0x00000400

#define AES_INPUT_BE		(AES_CNT_INPUT_ENDIAN)
#define AES_INPUT_LE		0
#define AES_INPUT_NORMAL	(AES_CNT_INPUT_ORDER)
#define AES_INPUT_REVERSED	0

#define AES_BLOCK_SIZE		0x10

#define AES_KEYCNT_WRITE	(1 << 0x7)
#define AES_KEYNORMAL		0
#define AES_KEYX		1
#define AES_KEYY		2

/**************************SHA****************************/
#define REG_SHA_CNT		((vu32 *)0x1000A000)
#define REG_SHA_BLKCNT		((vu32 *)0x1000A004)
#define REG_SHA_HASH		((vu32 *)0x1000A040)
#define REG_SHA_INFIFO		((vu32 *)0x1000A080)

#define SHA_CNT_STATE		0x00000003
#define SHA_CNT_UNK2		0x00000004
#define SHA_CNT_OUTPUT_ENDIAN	0x00000008
#define SHA_CNT_MODE		0x00000030
#define SHA_CNT_ENABLE		0x00010000
#define SHA_CNT_ACTIVE		0x00020000

#define SHA_HASH_READY		0x00000000
#define SHA_NORMAL_ROUND	0x00000001
#define SHA_FINAL_ROUND		0x00000002

#define SHA_OUTPUT_BE		SHA_CNT_OUTPUT_ENDIAN
#define SHA_OUTPUT_LE		0

#define SHA_256_MODE		0
#define SHA_224_MODE		0x00000010
#define SHA_1_MODE		0x00000020

#define SHA_256_HASH_SIZE	(256 / 8)
#define SHA_224_HASH_SIZE	(224 / 8)
#define SHA_1_HASH_SIZE		(160 / 8)

//NAND/FIRM stuff
void getNandCTR(void);
void readFirm0(u8 *outbuf, u32 size);
void writeFirm(u8 *inbuf, u32 offset, u32 size);
void setupKeyslot0x11(u32 a9lhBoot, const void *otp);
void generateSector(u8 *keySector);
void getSector(u8 *keySector);
u32 verifyHash(const void *data, u32 size, const u8 *hash);
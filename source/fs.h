/*
*   fs.h
*/

#pragma once

#include "types.h"

extern u32 console;

u32 mountSD(void);
u32 mountCTRNAND(void);
u32 fileRead(void *dest, const char *path);
void fileWrite(const void *buffer, const char *path, u32 size);
u32 firmRead(void *dest);
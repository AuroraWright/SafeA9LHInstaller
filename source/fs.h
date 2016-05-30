/*
*   fs.h
*/

#pragma once

#include "types.h"

u32 mountSD(void);
u32 fileWrite(void *orig, const char *path, u32 size);
u32 fileRead(void *dest, const char *path, u32 size);
u32 fileSize(const char *path);
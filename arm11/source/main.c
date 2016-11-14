#include "types.h"

void main(void)
{
    vu32 *arm11Entry = (vu32 *)0x1FFFFFF8;

    //Clear ARM11 entrypoint
    *arm11Entry = 0;

    //Wait for the entrypoint to be set
    while(!*arm11Entry);

    //Jump to it
    ((void (*)())*arm11Entry)();
}
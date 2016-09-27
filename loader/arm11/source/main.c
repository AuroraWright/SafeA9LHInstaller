#include "types.h"

void main(void)
{
    vu32 *arm11 = (vu32 *)0x1FFFFFF8;

    //Clear ARM11 entrypoint
    *arm11 = 0;

    //Wait for the entrypoint to be set
    while(!*arm11);

    //Jump to it
    ((void (*)())*arm11)();
}
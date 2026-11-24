#include "main.h"
#include "patch.h"

#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

void CHook::UnFuck(uintptr_t addr, size_t len)
{
#if VER_x32
    if(mprotect((void*)(addr & 0xFFFFF000), len, PROT_READ | PROT_WRITE | PROT_EXEC) == 0)
        return;

    mprotect((void*)(addr & 0xFFFFF000), len, PROT_READ | PROT_WRITE);
#else
    if(mprotect((void*)(addr & 0xFFFFFFFFFFFFF000), len, PROT_READ | PROT_WRITE | PROT_EXEC) == 0)
        return;

    mprotect((void*)(addr & 0xFFFFFFFFFFFFF000), len, PROT_READ | PROT_WRITE);
#endif
}

void CHook::InitHookStuff() {
    lib = dlopen("libGame.so", RTLD_LAZY);
}


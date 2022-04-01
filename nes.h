#ifndef _NES_H_
#define _NES_H_

#include "cpu.h"
#include "ppu.h"
#include "memory.h"
#include "ppu-memory.h"

typedef struct
{
    CPU *cpu;
    PPU *ppu;
    MEMORY *memory;
    PPU_MEMORY *ppu_memory;
    unsigned char interrupt_NMI;
} NES;

NES *create_nes();
void load_rom(NES *nes, const char *filename);
int execute_instruction(NES *nes);

#endif
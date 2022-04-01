#include <stdlib.h>
#include <stdio.h>

#include "ppu-memory.h"

PPU_MEMORY *create_ppu_memory()
{
    PPU_MEMORY *ppu_memory = malloc(sizeof(PPU_MEMORY));

    return ppu_memory;
}

unsigned char ppu_memory_read(PPU_MEMORY *ppu_memory, unsigned short address)
{
    if (address >= IMAGE_PALETTE)
    {
        return ppu_memory->palettes[address - IMAGE_PALETTE];
    }

    if (address >= NAME_TABLE_0)
    {
        return ppu_memory->name_tables[address - NAME_TABLE_0];
    }

    if (address >= PATTERN_TABLE_1)
    {
        return ppu_memory->pattern_table_1[address - PATTERN_TABLE_1];
    }

    return ppu_memory->pattern_table_0[address];
}

void ppu_memory_write(PPU_MEMORY *ppu_memory, unsigned short address, unsigned char value)
{
    if (address >= IMAGE_PALETTE)
    {
        ppu_memory->palettes[address - IMAGE_PALETTE] = value;
    }
    else if (address >= NAME_TABLE_0)
    {
        ppu_memory->name_tables[address - NAME_TABLE_0] = value;
    }
    else
    {
        printf("PPU write to %04X = %02X\n", address, value);
    }
}
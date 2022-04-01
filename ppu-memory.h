#ifndef _PPU_MEMORY_H_
#define _PPU_MEMORY_H_

#define PATTERN_TABLE_0 0x0000
#define PATTERN_TABLE_1 0x1000
#define NAME_TABLE_0 0x2000
#define NAME_TABLE_1 0x2400
#define NAME_TABLE_2 0x2800
#define NAME_TABLE_3 0x2C00

#define IMAGE_PALETTE 0x3F00
#define SPRITE_PALETTE 0x3F10

typedef struct
{
    unsigned char pattern_table_0[4 * 1024];
    unsigned char pattern_table_1[4 * 1024];
    unsigned char name_tables[4 * 0x400];
    unsigned char palettes[0x100];
} PPU_MEMORY;

PPU_MEMORY *create_ppu_memory();

unsigned char ppu_memory_read(PPU_MEMORY *ppu_memory, unsigned short address);
void ppu_memory_write(PPU_MEMORY *ppu_memory, unsigned short address, unsigned char value);

#endif
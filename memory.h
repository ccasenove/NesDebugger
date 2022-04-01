#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "ppu.h"

#define IO_REGISTERS 0x2000
#define EXPANSION_ROM 0x4020
#define PRG_ROM_UPPER_BANK 0xC000
#define PRG_ROM_LOWER_BANK 0x8000

#define NMI_ADDRESS 0xFFFA

#define SPRITE_DMA_REGISTER 0x4014

typedef struct
{
    unsigned char ram[2 * 1024];
    PPU *ppu;
    unsigned char prg_rom_lower_bank[16 * 1024];
    unsigned char prg_rom_upper_bank[16 * 1024];
    unsigned short last_read_address;
    unsigned short last_write_address;
} MEMORY;

MEMORY *create_memory(PPU *ppu);
unsigned char memory_read_byte(MEMORY *memory, unsigned short address);
unsigned short memory_read_word(MEMORY *memory, unsigned short address);
unsigned short memory_read_word_zero_page(MEMORY *memory, unsigned short address);
void memory_write(MEMORY *memory, unsigned short address, unsigned char value);

#endif
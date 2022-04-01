#ifndef _PPU_H_
#define _PPU_H_

#include "ppu-memory.h"

#define PPU_CONTROL_REGISTER 0x2000
#define PPU_MASK_REGISTER 0x2001
#define PPU_STATUS_REGISTER 0x2002
#define PPU_SPR_RAM_ADDRESS_REGISTER 0x2003
#define PPU_SPR_RAM_IO_REGISTER 0x2004
#define PPU_ADDRESS 0x2006
#define PPU_DATA 0x2007
#define NB_SPRITES 64

typedef struct
{
    PPU_MEMORY *ppu_memory;
    unsigned char spr_ram[NB_SPRITES * 4];
    unsigned char control_register;
    unsigned char mask_register;
    unsigned char status_register;
    unsigned short address;
    unsigned char address_write_low; // 0 = write to high address byte, 1 = write to low address byte
} PPU;

typedef struct
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} COLOR;

COLOR SYSTEM_PALETTE[64];

PPU *create_ppu(PPU_MEMORY *ppu_memory);
void update_ppu(PPU *ppu);
void ppu_write_address(PPU *ppu, unsigned char value);
void ppu_write_data(PPU *ppu, unsigned char value);

#endif
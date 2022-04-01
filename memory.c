#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"

MEMORY *create_memory(PPU *ppu)
{
    MEMORY *memory = malloc(sizeof(MEMORY));

    memory->ppu = ppu;
    memory->last_read_address = 0;
    memory->last_write_address = 0;

    return memory;
}

unsigned char memory_read_byte(MEMORY *memory, unsigned short address)
{
    memory->last_read_address = address;

    if (address >= PRG_ROM_UPPER_BANK)
    {
        return memory->prg_rom_upper_bank[address - PRG_ROM_UPPER_BANK];
    }

    if (address >= PRG_ROM_LOWER_BANK)
    {
        return memory->prg_rom_lower_bank[address - PRG_ROM_LOWER_BANK];
    }

    if (address >= EXPANSION_ROM)
    {
        printf("Expansion ROM / SRAM, memory read at %04X\n", address);
        return 0;
    }

    if (address >= IO_REGISTERS)
    {
        if (address == PPU_STATUS_REGISTER)
        {
            unsigned char status = memory->ppu->status_register;

            memory->ppu->status_register &= ~0x80;

            return status;
        }

        if (address == PPU_DATA)
        {
        }

        return 0;
    }

    return memory->ram[address];
}

unsigned short memory_read_word(MEMORY *memory, unsigned short address)
{
    return memory_read_byte(memory, address) | (memory_read_byte(memory, address + 1) << 8);
}

unsigned short memory_read_word_zero_page(MEMORY *memory, unsigned short address)
{
    return memory_read_byte(memory, address) | (memory_read_byte(memory, (address + 1) & 0xff) << 8);
}

void memory_write(MEMORY *memory, unsigned short address, unsigned char value)
{
    memory->last_write_address = address;

    if (address >= EXPANSION_ROM)
    {
        printf("Invalid write at %04X", address);
    }

    if (address >= IO_REGISTERS)
    {
        switch (address)
        {
        case PPU_CONTROL_REGISTER:
            memory->ppu->control_register = value;
            break;
        case PPU_MASK_REGISTER:
            memory->ppu->mask_register = value;
            break;
        case PPU_SPR_RAM_ADDRESS_REGISTER:
            printf("I/O Registers, memory write at $%04X = $%02X\n", address, value);
            break;
        case PPU_SPR_RAM_IO_REGISTER:
            printf("I/O Registers, memory write at $%04X = $%02X\n", address, value);
            break;
        case PPU_ADDRESS:
            ppu_write_address(memory->ppu, value);
            break;
        case PPU_DATA:
            ppu_write_data(memory->ppu, value);
            break;
        case SPRITE_DMA_REGISTER:
            printf("I/O Registers, memory write at $%04X = $%02X\n", address, value);
            memcpy(memory->ppu->spr_ram, memory->ram + 0x100 * value, sizeof(memory->ppu->spr_ram));
            break;
        }
    }
    else
    {
        memory->ram[address] = value;
    }
}

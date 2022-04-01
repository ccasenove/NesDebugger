/*
void draw_tile_bg(PPU *ppu, unsigned int tile_index, unsigned short pattern_table_address, unsigned short name_table_address, unsigned char tile, unsigned char *buffer)
{
    unsigned char x = (tile_index & 0x1f) << 3;
    unsigned char y = (tile_index & 0xffe0) >> 2;

    unsigned short tile_address = pattern_table_address + 16 * tile;
    unsigned int offset = y * SCREEN_WIDTH + x;

    unsigned char attribute_table_byte = ppu_memory_read(ppu->ppu_memory, name_table_address + 0x3c0 + ((tile_index & 0xff80) >> 4) + ((tile_index & 0x1f) >> 2));
    unsigned char color_offset = (~tile_index & 0x02) + ((tile_index & 0x40) >> 4);
    unsigned char palette_index = (attribute_table_byte >> color_offset) & 0x03;

    for (int i = 0; i < 8; i++)
    {
        unsigned char line_1 = ppu_memory_read(ppu->ppu_memory, tile_address + i);
        unsigned char line_2 = ppu_memory_read(ppu->ppu_memory, tile_address + 8 + i);
        for (int j = 7; j >= 0; j--)
        {
            unsigned char bit0 = (line_1 >> j) & 0x01;
            unsigned char bit1 = (line_2 >> j) & 0x01;
            unsigned char color_low = bit0 | (bit1 << 1);
            unsigned char color = ppu_memory_read(ppu->ppu_memory, IMAGE_PALETTE + 4 * palette_index + color_low);
            buffer[3 * (offset + 7 - j)] = SYSTEM_PALETTE[color].r;
            buffer[3 * (offset + 7 - j) + 1] = SYSTEM_PALETTE[color].g;
            buffer[3 * (offset + 7 - j) + 2] = SYSTEM_PALETTE[color].b;
        }
        offset += SCREEN_WIDTH;
    }
}
*/
#include <stdlib.h>
#include <stdio.h>

#include "nes.h"

#define LOG(...) printf(__VA_ARGS__)

char *dump_registers(CPU *cpu)
{
    char *str = malloc(26);
    sprintf(str, "A:%02X X:%02X Y:%02X P:%02X SP:%02X", cpu->registerA, cpu->registerX, cpu->registerY,
            cpu->registerP, cpu->sp);

    return str;
}

NES *create_nes()
{
    NES *nes = malloc(sizeof(NES));
    nes->ppu_memory = create_ppu_memory();
    nes->ppu = create_ppu(nes->ppu_memory);
    nes->memory = create_memory(nes->ppu);
    nes->cpu = create_cpu(nes->memory);

    return nes;
}

void load_rom(NES *nes, const char *filename)
{
    FILE *rom_file = fopen(filename, "rb");

    fseek(rom_file, 16, SEEK_SET);
    fread(nes->memory->prg_rom_lower_bank, 1, 16 * 1024, rom_file);
    fseek(rom_file, 16, SEEK_SET);
    fread(nes->memory->prg_rom_upper_bank, 1, 16 * 1024, rom_file);

    fread(nes->ppu_memory->pattern_table_0, 1, 4 * 1024, rom_file);
    fread(nes->ppu_memory->pattern_table_1, 1, 4 * 1024, rom_file);

    fclose(rom_file);

    nes->cpu->pc = memory_read_word(nes->memory, 0x0fffc);
}

void execute_rts(NES *nes)
{
    CPU *cpu = nes->cpu;
    MEMORY *memory = nes->memory;

    unsigned char lowAddress = pop(cpu);
    unsigned char highAddress = pop(cpu);
    cpu->pc = ((highAddress << 8) | lowAddress) + 1;
}

int execute_instruction(NES *nes)
{
    CPU *cpu = nes->cpu;
    MEMORY *memory = nes->memory;
    memory->last_read_address = 0;
    memory->last_write_address = 0;

    if (nes->interrupt_NMI && (nes->ppu->control_register & 0x80))
    {
        nes->interrupt_NMI = 0;
        trigger_NMI(cpu);
    }

    unsigned char inst = memory_read_byte(memory, cpu->pc);

    char *logBuffer;
    size_t logSize;
    FILE *logstream = open_memstream(&logBuffer, &logSize);

    char *logRegisters = dump_registers(cpu);
    fprintf(logstream, "%04X  %02X", cpu->pc, inst);
    cpu->pc++;

    unsigned short address;
    char disp;
    unsigned char value;
    unsigned short sum;
    unsigned short indirect_address;
    unsigned char carry;
    unsigned char zeropage_indexed_address;
    switch (inst)
    {
    case 0x01:
        // ORA indirect, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, (address + cpu->registerX) & 0xff);
        value = memory_read_byte(memory, indirect_address);
        cpu->registerA |= value;
        fprintf(logstream, " %02X     ", address & 0xff);
        fprintf(logstream, "ORA ($%02X,X) @ %02X = %04X = %02X", address & 0xff, (address + cpu->registerX) & 0xff, indirect_address, value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x05:
        // ORA zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ORA $%02X = %02X", address, value);
        cpu->registerA |= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x06:
        // ASL zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ASL $%02X = %02X", address, value);
        set_flag_cond(cpu, FLAG_C, value & 0x80);
        value = value << 1;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, address, value);
        break;
    case 0x08:
        // PHP
        fprintf(logstream, "        PHP");
        push(cpu, cpu->registerP | 0x30);
        break;
    case 0x09:
        // OR immediate
        value = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", value);
        fprintf(logstream, "ORA #$%02X", value);
        cpu->registerA |= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x0a:
        // ASL A
        fprintf(logstream, "        ASL A");
        set_flag_cond(cpu, FLAG_C, cpu->registerA & 0x80);
        cpu->registerA = cpu->registerA << 1;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x0d:
        // ORA absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "ORA $%04X = %02X", address, value);
        cpu->registerA |= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x0e:
        // ASL absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "ASL $%04X = %02X", address, value);
        set_flag_cond(cpu, FLAG_C, value & 0x80);
        value = value << 1;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, address, value);
        break;
    case 0x10:
        // BPL
        disp = memory_read_byte(memory, cpu->pc);
        cpu->pc += 1;
        fprintf(logstream, " %02X     ", disp);
        fprintf(logstream, "BPL $%04X", cpu->pc + disp);
        if (!get_flag(cpu, FLAG_N))
        {
            cpu->pc += disp;
        }
        break;
    case 0x11:
        // ORA indirect, Y
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, address);
        value = memory_read_byte(memory, indirect_address + cpu->registerY);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ORA ($%02X),Y = %04X @ %04X = %02X", address, indirect_address, (indirect_address + cpu->registerY) & 0xffff, value);
        cpu->registerA |= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x15:
        // ORA zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ORA $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        cpu->registerA |= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x16:
        // ASL zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ASL $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        set_flag_cond(cpu, FLAG_C, value & 0x80);
        value = value << 1;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, zeropage_indexed_address, value);
        break;
    case 0x18:
        // CLC
        clear_flag(cpu, FLAG_C);
        fprintf(logstream, "        CLC");
        break;
    case 0x19:
        // ORA absolute, Y
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerY);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "ORA $%04X,Y @ %04X = %02X", address, (address + cpu->registerY) & 0xffff, value);
        cpu->registerA |= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x1d:
        // ORA absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "ORA $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        cpu->registerA |= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x1e:
        // ASL absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "ASL $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        set_flag_cond(cpu, FLAG_C, value & 0x80);
        value = value << 1;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, address + cpu->registerX, value);
        break;
    case 0x20:
        // JSR
        address = memory_read_word(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "JSR $%04X", address);
        push(cpu, (cpu->pc >> 8) & 0xff);
        push(cpu, cpu->pc & 0xff);
        cpu->pc = address;
        break;
    case 0x21:
        // AND indirect, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, (address + cpu->registerX) & 0xff);
        value = memory_read_byte(memory, indirect_address);
        cpu->registerA &= value;
        fprintf(logstream, " %02X     ", address & 0xff);
        fprintf(logstream, "AND ($%02X,X) @ %02X = %04X = %02X", address & 0xff, (address + cpu->registerX) & 0xff, indirect_address, value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x24:
        // BIT zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "BIT $%02X = %02X", address, value);
        set_flag_cond(cpu, FLAG_Z, (cpu->registerA & value) == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        set_flag_cond(cpu, FLAG_V, value & 0x40);
        break;
    case 0x25:
        // AND zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "AND $%02X = %02X", address, value);
        cpu->registerA &= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x26:
        // ROL zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ROL $%02X = %02X", address, value);
        carry = get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_C, (value >> 7) & 0x01);
        value = (value << 1) | carry;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, address, value);
        break;
    case 0x27:
        // Undocumented: RLA zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address) << 1;
        memory_write(memory, address, value);
        cpu->registerA &= value;
        break;
    case 0x28:
        // PLP
        fprintf(logstream, "        PLP");
        cpu->registerP = (cpu->registerP & 0x30) | (pop(cpu) & 0xcf);
        break;
    case 0x29:
        // AND immediate
        value = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", value);
        fprintf(logstream, "AND #$%02X", value);
        cpu->registerA &= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x2a:
        // ROL A
        fprintf(logstream, "        ROL A");
        value = (cpu->registerA >> 7) & 0x01;
        cpu->registerA = (cpu->registerA << 1) | get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_C, value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x2c:
        // BIT absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "BIT $%04X = %02X", address, value);
        set_flag_cond(cpu, FLAG_Z, (cpu->registerA & value) == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        set_flag_cond(cpu, FLAG_V, value & 0x40);
        break;
    case 0x2d:
        // AND absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "AND $%04X = %02X", address, value);
        cpu->registerA &= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x2e:
        // ROL absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "ROL $%04X = %02X", address, value);
        carry = get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_C, (value >> 7) & 0x01);
        value = (value << 1) | carry;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, address, value);
        break;
    case 0x30:
        // BMI
        disp = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", disp);
        fprintf(logstream, "BMI $%04X", cpu->pc + disp);
        if (get_flag(cpu, FLAG_N))
        {
            cpu->pc += disp;
        }
        break;
    case 0x31:
        // AND indirect, Y
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, address);
        value = memory_read_byte(memory, indirect_address + cpu->registerY);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "AND ($%02X),Y = %04X @ %04X = %02X", address, indirect_address, (indirect_address + cpu->registerY) & 0xffff, value);
        cpu->registerA &= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x35:
        // AND zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "AND $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        cpu->registerA &= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x36:
        // ROL zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ROL $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        carry = get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_C, (value >> 7) & 0x01);
        value = (value << 1) | carry;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, zeropage_indexed_address, value);
        break;
    case 0x38:
        // SEC
        set_flag(cpu, FLAG_C);
        fprintf(logstream, "        SEC");
        break;
    case 0x39:
        // AND absolute, Y
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerY);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "AND $%04X,Y @ %04X = %02X", address, (address + cpu->registerY) & 0xffff, value);
        cpu->registerA &= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x3d:
        // AND absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "AND $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        cpu->registerA &= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x3e:
        // ROL absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "ROL $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        carry = get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_C, (value >> 7) & 0x01);
        value = (value << 1) | carry;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, address + cpu->registerX, value);
        break;
    case 0x40:
        // RTI
        fprintf(logstream, "        RTI");
        cpu->registerP = (cpu->registerP & 0x30) | (pop(cpu) & 0xcf);
        cpu->pc = pop(cpu) | (pop(cpu) << 8);
        break;
    case 0x41:
        // EOR indirect, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, (address + cpu->registerX) & 0xff);
        value = memory_read_byte(memory, indirect_address);
        cpu->registerA ^= value;
        fprintf(logstream, " %02X     ", address & 0xff);
        fprintf(logstream, "EOR ($%02X,X) @ %02X = %04X = %02X", address & 0xff, (address + cpu->registerX) & 0xff, indirect_address, value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x45:
        // EOR zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "EOR $%02X = %02X", address, value);
        cpu->registerA ^= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x46:
        // LSR zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "LSR $%02X = %02X", address, value);
        set_flag_cond(cpu, FLAG_C, value & 0x01);
        value = value >> 1;
        memory_write(memory, address, value);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        set_flag_cond(cpu, FLAG_Z, value == 0);
        break;
    case 0x48:
        // PHA
        fprintf(logstream, "        PHA");
        push(cpu, cpu->registerA);
        break;
    case 0x49:
        // EOR immediate
        value = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", value);
        fprintf(logstream, "EOR #$%02X", value);
        cpu->registerA ^= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x4a:
        // LSR A
        fprintf(logstream, "        LSR A");
        set_flag_cond(cpu, FLAG_C, cpu->registerA & 0x01);
        cpu->registerA = cpu->registerA >> 1;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        clear_flag(cpu, FLAG_N);
        break;
    case 0x4c:
        // JMP absolute
        address = memory_read_word(memory, cpu->pc);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "JMP $%04X", address);
        cpu->pc = address;
        break;
    case 0x4d:
        // EOR absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "EOR $%04X = %02X", address, value);
        cpu->registerA ^= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x4e:
        // LSR absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "LSR $%04X = %02X", address, value);
        set_flag_cond(cpu, FLAG_C, value & 0x01);
        value = value >> 1;
        memory_write(memory, address, value);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        set_flag_cond(cpu, FLAG_Z, value == 0);
        break;
    case 0x50:
        // BVC
        disp = memory_read_byte(memory, cpu->pc);
        cpu->pc += 1;
        fprintf(logstream, " %02X     ", disp);
        fprintf(logstream, "BVC $%04X", cpu->pc + disp);
        if (!get_flag(cpu, FLAG_V))
        {
            cpu->pc += disp;
        }
        break;
    case 0x51:
        // EOR indirect, Y
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, address);
        value = memory_read_byte(memory, indirect_address + cpu->registerY);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "EOR ($%02X),Y = %04X @ %04X = %02X", address, indirect_address, (indirect_address + cpu->registerY) & 0xffff, value);
        cpu->registerA ^= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x55:
        // EOR zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "EOR $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        cpu->registerA ^= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x56:
        // LSR zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "LSR $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        set_flag_cond(cpu, FLAG_C, value & 0x01);
        value = value >> 1;
        memory_write(memory, zeropage_indexed_address, value);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        set_flag_cond(cpu, FLAG_Z, value == 0);
        break;
    case 0x59:
        // EOR absolute, Y
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerY);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "EOR $%04X,Y @ %04X = %02X", address, (address + cpu->registerY) & 0xffff, value);
        cpu->registerA ^= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x5d:
        // EOR absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "EOR $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        cpu->registerA ^= value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0x5e:
        // LSR absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "LSR $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        set_flag_cond(cpu, FLAG_C, value & 0x01);
        value = value >> 1;
        memory_write(memory, address + cpu->registerX, value);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        set_flag_cond(cpu, FLAG_Z, value == 0);
        break;
    case 0x60:
        // RTS
        fprintf(logstream, "        RTS");
        execute_rts(nes);
        break;
    case 0x61:
        // ADC indirect, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, (address + cpu->registerX) & 0xff);
        value = memory_read_byte(memory, indirect_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ADC ($%02X,X) @ %02X = %04X = %02X", address, (address + cpu->registerX) & 0xff, indirect_address, value);
        sum = cpu->registerA + value + get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_V, (value < 0x80 && cpu->registerA < 0x80 && sum > 0x7f) || (value >= 0x80 && cpu->registerA >= 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum > 0xff);
        break;
    case 0x65:
        // ADC zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ADC $%02X = %02X", address, value);
        sum = cpu->registerA + value + get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_V, (value < 0x80 && cpu->registerA < 0x80 && sum > 0x7f) || (value >= 0x80 && cpu->registerA >= 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum > 0xff);
        break;
    case 0x66:
        // ROR zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ROR $%02X = %02X", address, value);
        carry = get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_C, value & 0x01);
        value = (value >> 1) | (carry << 7);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        set_flag_cond(cpu, FLAG_Z, value == 0);
        memory_write(memory, address, value);
        break;
    case 0x68:
        // PLA
        fprintf(logstream, "        PLA");
        cpu->registerA = pop(cpu);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        break;
    case 0x69:
        // ADC immediate
        value = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", value);
        fprintf(logstream, "ADC #$%02X", value);
        sum = cpu->registerA + value + get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_V, (value < 0x80 && cpu->registerA < 0x80 && sum > 0x7f) || (value >= 0x80 && cpu->registerA >= 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum > 0xff);
        break;
    case 0x6a:
        // ROR A
        fprintf(logstream, "        ROR A");
        value = get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_C, cpu->registerA & 0x01);
        cpu->registerA = (cpu->registerA >> 1) | (value << 7);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        break;
    case 0x6c:
        // JMP indirect
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        indirect_address = memory_read_byte(memory, address) | (memory_read_byte(memory, ((address & 0xff00) + ((address + 1) & 0xff))) << 8);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "JMP ($%04X) = %04X", address, indirect_address);
        cpu->pc = indirect_address;
        break;
    case 0x6d:
        // ADC absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "ADC $%04X = %02X", address, value);
        sum = cpu->registerA + value + get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_V, (value < 0x80 && cpu->registerA < 0x80 && sum > 0x7f) || (value >= 0x80 && cpu->registerA >= 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum > 0xff);
        break;
    case 0x6e:
        // ROR absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "ROR $%04X = %02X", address, value);
        carry = get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_C, value & 0x01);
        value = (value >> 1) | (carry << 7);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        set_flag_cond(cpu, FLAG_Z, value == 0);
        memory_write(memory, address, value);
        break;
    case 0x70:
        // BVS
        disp = memory_read_byte(memory, cpu->pc);
        cpu->pc += 1;
        fprintf(logstream, " %02X     ", disp);
        fprintf(logstream, "BVS $%04X", cpu->pc + disp);
        if (get_flag(cpu, FLAG_V))
        {
            cpu->pc += disp;
        }
        break;
    case 0x71:
        // ADC indirect, Y
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, address);
        value = memory_read_byte(memory, indirect_address + cpu->registerY);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ADC ($%02X),Y = %04X @ %04X = %02X", address, indirect_address, (indirect_address + cpu->registerY) & 0xffff, value);
        sum = cpu->registerA + value + get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_V, (value < 0x80 && cpu->registerA < 0x80 && sum > 0x7f) || (value >= 0x80 && cpu->registerA >= 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum > 0xff);
        break;
    case 0x75:
        // ADC zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ADC $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        sum = cpu->registerA + value + get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_V, (value < 0x80 && cpu->registerA < 0x80 && sum > 0x7f) || (value >= 0x80 && cpu->registerA >= 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum > 0xff);
        break;
    case 0x76:
        // ROR zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "ROR $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        carry = get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_C, value & 0x01);
        value = (value >> 1) | (carry << 7);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        set_flag_cond(cpu, FLAG_Z, value == 0);
        memory_write(memory, zeropage_indexed_address, value);
        break;
    case 0x78:
        // SEI
        fprintf(logstream, "        SEI");
        set_flag(cpu, FLAG_I);
        break;
    case 0x79:
        // ADC absolute, Y
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerY);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "ADC $%04X,Y @ %04X = %02X", address, (address + cpu->registerY) & 0xffff, value);
        sum = cpu->registerA + value + get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_V, (value < 0x80 && cpu->registerA < 0x80 && sum > 0x7f) || (value >= 0x80 && cpu->registerA >= 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum > 0xff);
        break;
    case 0x7d:
        // ADC absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "ADC $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        sum = cpu->registerA + value + get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_V, (value < 0x80 && cpu->registerA < 0x80 && sum > 0x7f) || (value >= 0x80 && cpu->registerA >= 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum > 0xff);
        break;
    case 0x7e:
        // ROR absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "ROR $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        carry = get_flag(cpu, FLAG_C);
        set_flag_cond(cpu, FLAG_C, value & 0x01);
        value = (value >> 1) | (carry << 7);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        set_flag_cond(cpu, FLAG_Z, value == 0);
        memory_write(memory, address + cpu->registerX, value);
        break;
    case 0x81:
        // STA (indirect, X)
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, (address + cpu->registerX) & 0xff);
        fprintf(logstream, " %02X     ", address & 0xff);
        fprintf(logstream, "STA ($%02X,X) @ %02X = %04X = %02X", address & 0xff, (address + cpu->registerX) & 0xff, indirect_address, memory_read_byte(memory, indirect_address));
        memory_write(memory, indirect_address, cpu->registerA);
        break;
    case 0x84:
        // STY zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "STY $%02X = %02X", address, memory_read_byte(memory, address));
        memory_write(memory, address, cpu->registerY);
        break;
    case 0x85:
        // STA zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "STA $%02X = %02X", address, memory_read_byte(memory, address));
        memory_write(memory, address, cpu->registerA);
        break;
    case 0x86:
        // STX zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "STX $%02X = %02X", address, memory_read_byte(memory, address));
        memory_write(memory, address, cpu->registerX);
        break;
    case 0x88:
        // DEY
        fprintf(logstream, "        DEY");
        cpu->registerY--;
        set_flag_cond(cpu, FLAG_N, cpu->registerY & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerY == 0);
        break;
    case 0x8a:
        // TXA
        fprintf(logstream, "        TXA");
        cpu->registerA = cpu->registerX;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        break;
    case 0x8c:
        // STY absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "STY $%04X = %02X", address, memory_read_byte(memory, address));
        memory_write(memory, address, cpu->registerY);
        break;
    case 0x8d:
        // STA absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "STA $%04X = %02X", address, memory_read_byte(memory, address));
        memory_write(memory, address, cpu->registerA);
        break;
    case 0x8e:
        // STX absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "STX $%04X = %02X", address, memory_read_byte(memory, address));
        memory_write(memory, address, cpu->registerX);
        break;
    case 0x90:
        // BCC
        disp = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", disp);
        fprintf(logstream, "BCC $%04X", cpu->pc + disp);
        if (!get_flag(cpu, FLAG_C))
        {
            cpu->pc += disp;
        }
        break;
    case 0x91:
        // STA indirect, Y
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, address);
        value = memory_read_byte(memory, indirect_address + cpu->registerY);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "STA ($%02X),Y = %04X @ %04X = %02X", address, indirect_address, (indirect_address + cpu->registerY) & 0xffff, value);
        memory_write(memory, indirect_address + cpu->registerY, cpu->registerA);
        break;
    case 0x94:
        // STY zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "STY $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        memory_write(memory, zeropage_indexed_address, cpu->registerY);
        break;
    case 0x95:
        // STA zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "STA $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        memory_write(memory, zeropage_indexed_address, cpu->registerA);
        break;
    case 0x96:
        // STX zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerY;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "STX $%02X,Y @ %02X = %02X", address, zeropage_indexed_address, value);
        memory_write(memory, zeropage_indexed_address, cpu->registerX);
        break;
    case 0x98:
        // TYA
        fprintf(logstream, "        TYA");
        cpu->registerA = cpu->registerY;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        break;
    case 0x99:
        // STA absolute, Y
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerY);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "STA $%04X,Y @ %04X = %02X", address, (address + cpu->registerY) & 0xffff, value);
        memory_write(memory, address + cpu->registerY, cpu->registerA);
        break;
    case 0x9a:
        // TXS
        fprintf(logstream, "        TXS");
        cpu->sp = cpu->registerX;
        break;
    case 0x9d:
        // STA absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "STA $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        memory_write(memory, address + cpu->registerX, cpu->registerA);
        break;
    case 0xa0:
        // LDY immediate
        cpu->registerY = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", cpu->registerY);
        fprintf(logstream, "LDY #$%02X", cpu->registerY);
        set_flag_cond(cpu, FLAG_Z, cpu->registerY == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerY & 0x80);
        break;
    case 0xa1:
        // LDA (indirect, X)
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, (address + cpu->registerX) & 0xff);
        cpu->registerA = memory_read_byte(memory, indirect_address);
        fprintf(logstream, " %02X     ", address & 0xff);
        fprintf(logstream, "LDA ($%02X,X) @ %02X = %04X = %02X", address & 0xff, (address + cpu->registerX) & 0xff, indirect_address, cpu->registerA);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0xa2:
        // LDX immediate
        cpu->registerX = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", cpu->registerX);
        fprintf(logstream, "LDX #$%02X", cpu->registerX);
        set_flag_cond(cpu, FLAG_Z, cpu->registerX == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerX & 0x80);
        break;
    case 0xa4:
        // LDY zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        cpu->registerY = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "LDY $%02X = %02X", address, cpu->registerY);
        set_flag_cond(cpu, FLAG_Z, cpu->registerY == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerY & 0x80);
        break;
    case 0xa5:
        // LDA zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        cpu->registerA = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "LDA $%02X = %02X", address, cpu->registerA);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0xa6:
        // LDX zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        cpu->registerX = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "LDX $%02X = %02X", address, cpu->registerX);
        set_flag_cond(cpu, FLAG_Z, cpu->registerX == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerX & 0x80);
        break;
    case 0xa8:
        // TAY
        cpu->registerY = cpu->registerA;
        fprintf(logstream, "        TAY");
        set_flag_cond(cpu, FLAG_Z, cpu->registerY == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerY & 0x80);
        break;
    case 0xa9:
        // LDA immediate
        cpu->registerA = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", cpu->registerA);
        fprintf(logstream, "LDA #$%02X", cpu->registerA);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0xaa:
        // TAX
        fprintf(logstream, "        TAX");
        cpu->registerX = cpu->registerA;
        set_flag_cond(cpu, FLAG_N, cpu->registerX & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerX == 0);
        break;
    case 0xac:
        // LDY absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        cpu->registerY = memory_read_byte(memory, address);
        fprintf(logstream, "LDY $%04X = %02X", address, cpu->registerY);
        set_flag_cond(cpu, FLAG_Z, cpu->registerY == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerY & 0x80);
        break;
    case 0xad:
        // LDA absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        cpu->registerA = memory_read_byte(memory, address);
        fprintf(logstream, "LDA $%04X = %02X", address, cpu->registerA);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0xae:
        // LDX absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        cpu->registerX = memory_read_byte(memory, address);
        fprintf(logstream, "LDX $%04X = %02X", address, cpu->registerX);
        set_flag_cond(cpu, FLAG_Z, cpu->registerX == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerX & 0x80);
        break;
    case 0xb0:
        // BCS
        disp = memory_read_byte(memory, cpu->pc);
        cpu->pc += 1;
        fprintf(logstream, " %02X     ", disp);
        fprintf(logstream, "BCS $%04X", cpu->pc + disp);
        if (get_flag(cpu, FLAG_C))
        {
            cpu->pc += disp;
        }
        break;
    case 0xb1:
        // LDA indirect, Y
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, address);
        value = memory_read_byte(memory, indirect_address + cpu->registerY);
        fprintf(logstream, " %02X     ", address & 0xff);
        fprintf(logstream, "LDA ($%02X),Y = %04X @ %04X = %02X", address, indirect_address, (indirect_address + cpu->registerY) & 0xffff, value);
        cpu->registerA = value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0xb4:
        // LDY zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        cpu->registerY = memory_read_byte(memory, (address + cpu->registerX) & 0xff);
        fprintf(logstream, " %02X     ", address & 0xff);
        fprintf(logstream, "LDY $%02X,X @ %02X = %02X", address, (address + cpu->registerX) & 0xff, cpu->registerY);
        set_flag_cond(cpu, FLAG_Z, cpu->registerY == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerY & 0x80);
        break;
    case 0xb5:
        // LDA zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        cpu->registerA = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "LDA $%02X,X @ %02X = %02X", address, zeropage_indexed_address, cpu->registerA);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0xb6:
        // LDX zero page, Y
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerY;
        cpu->registerX = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "LDX $%02X,Y @ %02X = %02X", address, zeropage_indexed_address, cpu->registerX);
        set_flag_cond(cpu, FLAG_Z, cpu->registerX == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerX & 0x80);
        break;
    case 0xb8:
        // CLV
        fprintf(logstream, "        CLV");
        clear_flag(cpu, FLAG_V);
        break;
    case 0xb9:
        // LDA absolute, Y
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerY);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "LDA $%04X,Y @ %04X = %02X", address, (address + cpu->registerY) & 0xffff, value);
        cpu->registerA = value;
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0xba:
        // TSX
        fprintf(logstream, "        TSX");
        cpu->registerX = cpu->sp;
        set_flag_cond(cpu, FLAG_Z, cpu->registerX == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerX & 0x80);
        break;
    case 0xbc:
        // LDY absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        cpu->registerY = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "LDY $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, cpu->registerY);
        set_flag_cond(cpu, FLAG_Z, cpu->registerY == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerY & 0x80);
        break;
    case 0xbd:
        // LDA absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        cpu->registerA = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "LDA $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, cpu->registerA);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        break;
    case 0xbe:
        // LDX absolute, Y
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        cpu->registerX = memory_read_byte(memory, address + cpu->registerY);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "LDX $%04X,Y @ %04X = %02X", address, (address + cpu->registerY) & 0xffff, cpu->registerX);
        set_flag_cond(cpu, FLAG_Z, cpu->registerX == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerX & 0x80);
        break;
    case 0xc0:
        // CPY immediate
        value = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", value);
        fprintf(logstream, "CPY #$%02X", value);
        set_flag_cond(cpu, FLAG_C, cpu->registerY >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerY == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerY - value) & 0x80);
        break;
    case 0xc1:
        // CMP indirect, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, (address + cpu->registerX) & 0xff);
        value = memory_read_byte(memory, indirect_address);
        fprintf(logstream, " %02X     ", address & 0xff);
        fprintf(logstream, "CMP ($%02X,X) @ %02X = %04X = %02X", address, (address + cpu->registerX) & 0xff, indirect_address, value);
        set_flag_cond(cpu, FLAG_C, cpu->registerA >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerA - value) & 0x80);
        break;
    case 0xc4:
        // CPY zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "CPY $%02X = %02X", address, value);
        set_flag_cond(cpu, FLAG_C, cpu->registerY >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerY == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerY - value) & 0x80);
        break;
    case 0xc5:
        // CMP zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "CMP $%02X = %02X", address, value);
        set_flag_cond(cpu, FLAG_C, cpu->registerA >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerA - value) & 0x80);
        break;
    case 0xc6:
        // DEC zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "DEC $%02X = %02X", address, value);
        value--;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, address, value);
        break;
    case 0xc8:
        // INY
        fprintf(logstream, "        INY");
        cpu->registerY++;
        set_flag_cond(cpu, FLAG_Z, cpu->registerY == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerY & 0x80);
        break;
    case 0xc9:
        // CMP immediate
        value = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", value);
        fprintf(logstream, "CMP #$%02X", value);
        set_flag_cond(cpu, FLAG_C, cpu->registerA >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerA - value) & 0x80);
        break;
    case 0xca:
        // DEX
        fprintf(logstream, "        DEX");
        cpu->registerX--;
        set_flag_cond(cpu, FLAG_Z, cpu->registerX == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerX & 0x80);
        break;
    case 0xcc:
        // CPY absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "CPY $%04X = %02X", address, value);
        set_flag_cond(cpu, FLAG_C, cpu->registerY >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerY == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerY - value) & 0x80);
        break;
    case 0xcd:
        // CMP absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "CMP $%04X = %02X", address, value);
        set_flag_cond(cpu, FLAG_C, cpu->registerA >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerA - value) & 0x80);
        break;
    case 0xce:
        // DEC absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "DEC $%04X = %02X", address, value);
        value--;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, address, value);
        break;
    case 0xd0:
        // BNE
        disp = memory_read_byte(memory, cpu->pc);
        cpu->pc += 1;
        fprintf(logstream, " %02X     ", disp);
        fprintf(logstream, "BNE $%02X", cpu->pc + disp);
        if (!get_flag(cpu, FLAG_Z))
        {
            cpu->pc += disp;
        }
        break;
    case 0xd1:
        // CMP indirect, Y
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, address);
        value = memory_read_byte(memory, indirect_address + cpu->registerY);
        fprintf(logstream, " %02X     ", address & 0xff);
        fprintf(logstream, "CMP ($%02X),Y = %04X @ %04X = %02X", address, indirect_address, (indirect_address + cpu->registerY) & 0xffff, value);
        set_flag_cond(cpu, FLAG_C, cpu->registerA >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerA - value) & 0x80);
        break;
    case 0xd5:
        // CMP zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "CMP $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        set_flag_cond(cpu, FLAG_C, cpu->registerA >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerA - value) & 0x80);
        break;
    case 0xd6:
        // DEC zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "DEC $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        value--;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, zeropage_indexed_address, value);
        break;
    case 0xd8:
        // CLD
        fprintf(logstream, "        CLD");
        clear_flag(cpu, FLAG_D);
        break;
    case 0xd9:
        // CMP absolute, Y
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerY);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "CMP $%04X,Y @ %04X = %02X", address, (address + cpu->registerY) & 0xffff, value);
        set_flag_cond(cpu, FLAG_C, cpu->registerA >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerA - value) & 0x80);
        break;
    case 0xdd:
        // CMP absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "CMP $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        set_flag_cond(cpu, FLAG_C, cpu->registerA >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerA - value) & 0x80);
        break;
    case 0xde:
        // DEC absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "DEC $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        value--;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, address + cpu->registerX, value);
        break;
    case 0xe0:
        // CPX immediate
        value = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", value);
        fprintf(logstream, "CPX #$%02X", value);
        set_flag_cond(cpu, FLAG_C, cpu->registerX >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerX == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerX - value) & 0x80);
        break;
    case 0xe1:
        // SBC indirect, x
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, (address + cpu->registerX) & 0xff);
        value = memory_read_byte(memory, indirect_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "SBC ($%02X,X) @ %02X = %04X = %02X", address, (address + cpu->registerX) & 0xff, indirect_address, value);
        sum = cpu->registerA - value - (~get_flag(cpu, FLAG_C) & 0x01);
        set_flag_cond(cpu, FLAG_V, (cpu->registerA < 0x80 && value >= 0x80 && sum >= 0x80) || (cpu->registerA >= 0x80 && value < 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum < 0x80);
        break;
    case 0xe4:
        // CPX zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "CPX $%02X = %02X", address, value);
        set_flag_cond(cpu, FLAG_C, cpu->registerX >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerX == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerX - value) & 0x80);
        break;
    case 0xe5:
        // SBC zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "SBC $%02X = %02X", address, value);
        sum = cpu->registerA - value - (~get_flag(cpu, FLAG_C) & 0x01);
        set_flag_cond(cpu, FLAG_V, (cpu->registerA < 0x80 && value >= 0x80 && sum >= 0x80) || (cpu->registerA >= 0x80 && value < 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum < 0x80);
        break;
    case 0xe6:
        // INC zero page
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "INC $%02X = %02X", address, value);
        value++;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, address, value);
        break;
    case 0xe8:
        // INCX
        fprintf(logstream, "        INX");
        cpu->registerX++;
        set_flag_cond(cpu, FLAG_Z, cpu->registerX == 0);
        set_flag_cond(cpu, FLAG_N, cpu->registerX & 0x80);
        break;
    case 0xe9:
        // SBC immediate
        value = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        fprintf(logstream, " %02X     ", value);
        fprintf(logstream, "SBC #$%02X", value);
        sum = cpu->registerA - value - (~get_flag(cpu, FLAG_C) & 0x01);
        set_flag_cond(cpu, FLAG_V, (cpu->registerA < 0x80 && value >= 0x80 && sum >= 0x80) || (cpu->registerA >= 0x80 && value < 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum < 0x80);
        break;
    case 0xea:
        // NOP
        fprintf(logstream, "        NOP");
        break;
    case 0xec:
        // CPX absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "CPX $%04X = %02X", address, value);
        set_flag_cond(cpu, FLAG_C, cpu->registerX >= value);
        set_flag_cond(cpu, FLAG_Z, cpu->registerX == value);
        set_flag_cond(cpu, FLAG_N, (cpu->registerX - value) & 0x80);
        break;
    case 0xed:
        // SBC absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "SBC $%04X = %02X", address, value);
        sum = cpu->registerA - value - (~get_flag(cpu, FLAG_C) & 0x01);
        set_flag_cond(cpu, FLAG_V, (cpu->registerA < 0x80 && value >= 0x80 && sum >= 0x80) || (cpu->registerA >= 0x80 && value < 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum < 0x80);
        break;
    case 0xee:
        // INC absolute
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "INC $%04X = %02X", address, value);
        value++;
        memory_write(memory, address, value);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        set_flag_cond(cpu, FLAG_Z, value == 0);
        break;
    case 0xf0:
        // BEQ
        disp = memory_read_byte(memory, cpu->pc);
        cpu->pc += 1;
        fprintf(logstream, " %02X     ", disp);
        fprintf(logstream, "BEQ $%02X", cpu->pc + disp);
        if (get_flag(cpu, FLAG_Z))
        {
            cpu->pc += disp;
        }
        break;
    case 0xf1:
        // SBC indirect, y
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        indirect_address = memory_read_word_zero_page(memory, address);
        value = memory_read_byte(memory, indirect_address + cpu->registerY);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "SBC ($%02X),Y = %04X @ %04X = %02X", address, indirect_address, (indirect_address + cpu->registerY) & 0xffff, value);
        sum = cpu->registerA - value - (~get_flag(cpu, FLAG_C) & 0x01);
        set_flag_cond(cpu, FLAG_V, (cpu->registerA < 0x80 && value >= 0x80 && sum >= 0x80) || (cpu->registerA >= 0x80 && value < 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum < 0x80);
        break;
    case 0xf5:
        // SBC zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "SBC $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        sum = cpu->registerA - value - (~get_flag(cpu, FLAG_C) & 0x01);
        set_flag_cond(cpu, FLAG_V, (cpu->registerA < 0x80 && value >= 0x80 && sum >= 0x80) || (cpu->registerA >= 0x80 && value < 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum < 0x80);
        break;
    case 0xf6:
        // INC zero page, X
        address = memory_read_byte(memory, cpu->pc);
        cpu->pc++;
        zeropage_indexed_address = address + cpu->registerX;
        value = memory_read_byte(memory, zeropage_indexed_address);
        fprintf(logstream, " %02X     ", address);
        fprintf(logstream, "INC $%02X,X @ %02X = %02X", address, zeropage_indexed_address, value);
        value++;
        set_flag_cond(cpu, FLAG_Z, value == 0);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        memory_write(memory, zeropage_indexed_address, value);
        break;
    case 0xf8:
        // SED
        fprintf(logstream, "        SED");
        set_flag(cpu, FLAG_D);
        break;
    case 0xf9:
        // SBC absolute, Y
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerY);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "SBC $%04X,Y @ %04X = %02X", address, (address + cpu->registerY) & 0xffff, value);
        sum = cpu->registerA - value - (~get_flag(cpu, FLAG_C) & 0x01);
        set_flag_cond(cpu, FLAG_V, (cpu->registerA < 0x80 && value >= 0x80 && sum >= 0x80) || (cpu->registerA >= 0x80 && value < 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum < 0x80);
        break;
    case 0xfd:
        // SBC absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "SBC $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        sum = cpu->registerA - value - (~get_flag(cpu, FLAG_C) & 0x01);
        set_flag_cond(cpu, FLAG_V, (cpu->registerA < 0x80 && value >= 0x80 && sum >= 0x80) || (cpu->registerA >= 0x80 && value < 0x80 && sum < 0x80));
        cpu->registerA = sum & 0xff;
        set_flag_cond(cpu, FLAG_N, cpu->registerA & 0x80);
        set_flag_cond(cpu, FLAG_Z, cpu->registerA == 0);
        set_flag_cond(cpu, FLAG_C, sum < 0x80);
        break;
    case 0xfe:
        // INC absolute, X
        address = memory_read_word(memory, cpu->pc);
        cpu->pc += 2;
        value = memory_read_byte(memory, address + cpu->registerX);
        fprintf(logstream, " %02X %02X  ", address & 0xff, (address >> 8) & 0xff);
        fprintf(logstream, "INC $%04X,X @ %04X = %02X", address, (address + cpu->registerX) & 0xffff, value);
        value++;
        memory_write(memory, address + cpu->registerX, value);
        set_flag_cond(cpu, FLAG_N, value & 0x80);
        set_flag_cond(cpu, FLAG_Z, value == 0);
        break;
    default:
        fprintf(stderr, "Illegal instruction 0x%02x\n", inst);
        return -1;
    }

    fclose(logstream);

    printf("%-48s%s\n", logBuffer, logRegisters);
    // printf("$%04X\n", memory_read_word(memory, 0x02));
    free(logBuffer);

    return 0;
}
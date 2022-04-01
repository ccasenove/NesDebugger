#include <stdlib.h>
#include "cpu.h"

CPU *create_cpu(MEMORY *memory)
{
    CPU *cpu = malloc(sizeof(CPU));
    cpu->memory = memory;

    cpu->registerA = 0;
    cpu->registerX = 0;
    cpu->registerY = 0;
    cpu->registerP = 0x24;
    cpu->sp = 0xfd;

    return cpu;
}

void set_flag(CPU *cpu, unsigned char flag)
{
    cpu->registerP |= flag;
}

void set_flag_cond(CPU *cpu, unsigned char flag, unsigned char condition)
{
    if (condition)
    {
        set_flag(cpu, flag);
    }
    else
    {
        clear_flag(cpu, flag);
    }
}

void clear_flag(CPU *cpu, unsigned char flag)
{
    cpu->registerP &= ~flag;
}

unsigned char get_flag(CPU *cpu, unsigned char flag)
{
    return cpu->registerP & flag;
}

void push(CPU *cpu, unsigned char value)
{
    cpu->memory->ram[STACK_BASE + cpu->sp] = value;
    cpu->sp--;
}

unsigned char pop(CPU *cpu)
{
    cpu->sp++;
    return cpu->memory->ram[STACK_BASE + cpu->sp];
}

void trigger_NMI(CPU *cpu)
{
    push(cpu, cpu->pc >> 8);
    push(cpu, cpu->pc & 0xff);
    push(cpu, cpu->registerP);
    cpu->pc = memory_read_word(cpu->memory, NMI_ADDRESS);
}
#ifndef _CPU_H_
#define _CPU_H_

#include "memory.h"

#define FLAG_C 0x01
#define FLAG_Z 0x02
#define FLAG_I 0x04
#define FLAG_D 0x08
#define FLAG_B 0x10
#define FLAG_V 0x40
#define FLAG_N 0x80

#define STACK_BASE 0x100

typedef struct
{
    unsigned char registerA;
    unsigned char registerX;
    unsigned char registerY;
    unsigned char registerP;
    unsigned short pc;
    unsigned char sp;

    MEMORY *memory;
} CPU;

CPU *create_cpu(MEMORY *memory);

void set_flag(CPU *cpu, unsigned char flag);
void set_flag_cond(CPU *cpu, unsigned char flag, unsigned char condition);
void clear_flag(CPU *cpu, unsigned char flag);
unsigned char get_flag(CPU *cpu, unsigned char flag);

void push(CPU *cpu, unsigned char value);
unsigned char pop(CPU *cpu);
void trigger_NMI(CPU *cpu);

#endif
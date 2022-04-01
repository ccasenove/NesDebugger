#ifndef _DISASSEMBLER_H_
#define _DISASSEMBLER_H_

enum ADDRESSING_MODE
{
    IMPLIED,
    ACCUMULATOR,
    IMMEDIATE,
    ZERO_PAGE,
    ZERO_PAGE_X,
    ZERO_PAGE_Y,
    RELATIVE,
    ABSOLUTE,
    ABSOLUTE_X,
    ABSOLUTE_Y,
    INDIRECT,
    INDEXED_INDIRECT,
    INDIRECT_INDEXED
};

typedef struct
{
    const char *mnemonic;
    enum ADDRESSING_MODE addressing_mode;
} INSTRUCTION_INFO;

typedef struct
{
    unsigned char opcode;
    unsigned char length;
    const char *mnemonic;
    enum ADDRESSING_MODE addressing_mode;
    unsigned char value;
    char displacement;
    unsigned short address;
} INSTRUCTION;

void dis_parse_instruction(unsigned char byte1, unsigned char byte2, unsigned char byte3,
                           INSTRUCTION *instruction);

void dis_instruction_to_str(INSTRUCTION *instruction, unsigned short pc, char *str);

#endif
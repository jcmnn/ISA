#ifndef INSTRUCTIONSX86_H
#define INSTRUCTIONSX86_H
#include <cstdint>

enum Opcodex86Type
{
    OPT_1BYTE,
    OPT_2BYTES,
    OPT_3BYTES,
};

struct Instructionx86
{
    Opcodex86Type type;
    uint32_t opcode;
    
};


extern Instructionx86 *instructions;

#endif

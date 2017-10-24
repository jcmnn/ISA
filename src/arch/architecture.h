#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H
#include "disasm/instructioninfo.h"
#include <string>
#include <vector>

class Architecture
{
public:
    struct Token
    {
        enum Type
        {
            TYPE_MNEMONIC,
            TYPE_TEXT,
            TYPE_ADDRESS,
            TYPE_CONSTANT,
            TYPE_REGISTER,
        };
        
        enum ValueType
        {
            TYPE_STRING,
            TYPE_INTEGER,
        };
        
        ValueType valueType;
        Type type;
        std::string text;
        union {
            uint64_t address;
            uint16_t reg;
            int64_t constant;
        };

    };
    
    
    Architecture();
    
    /* Gets the text to be displayed in the disassembly view for an instruction.
     * Returns true when the instruction was disassembled and tokens is populated */
    virtual bool instructionText(uint64_t instructionPointer, const char *data, size_t size, std::vector<Token> &tokens) =0;
    
    /* Gets information such as branching data and length from an instruction.
     * Returns true when the instruction was disassembled and iinfo is populated */
    virtual bool instructionInfo(uint64_t instructionPointer, const char *data, size_t size, InstructionInfo &iinfo) =0;
    
    
    /* Gets the register name from a register id. Returns true
     * if the register exists and the name was set */
    virtual bool registerName(uint16_t id, std::string &name) =0;
    
};

#endif // ARCHITECTURE_H

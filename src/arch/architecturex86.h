#ifndef ARCHITECTUREX86_H
#define ARCHITECTUREX86_H
#include "architecture.h"
#include "Zydis/Zydis.h"

class Architecturex86 : public Architecture
{
public:
    enum Mode
    {
        BIT16,
        BIT32,
        BIT64
    };
    
    Architecturex86(Mode mode);
    
    bool instructionInfo(uint64_t instructionPointer, const char * data, std::size_t size, InstructionInfo & iinfo) override;
    
    bool instructionText(uint64_t instructionPointer, const char * data, std::size_t size, std::vector<Token> & tokens) override;
    
    bool registerName(uint16_t id, std::string & name) override;
    
    
    inline bool valid()
    {
        return valid_;
    }
    
    
    /* Zydis formatter hooks */
    static ZydisStatus hookFormatOperandReg(const ZydisFormatter* formatter, 
                                            char** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, 
                                            const ZydisDecodedOperand* operand, void* userData);
    
    static ZydisStatus hookFormatOperandMem(const ZydisFormatter* formatter, 
                                            char** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, 
                                            const ZydisDecodedOperand* operand, void* userData);
    
    static ZydisStatus hookFormatOperandPtr(const ZydisFormatter* formatter, 
                                            char** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, 
                                            const ZydisDecodedOperand* operand, void* userData);
    
    static ZydisStatus hookFormatOperandImm(const ZydisFormatter* formatter, 
                                            char** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, 
                                            const ZydisDecodedOperand* operand, void* userData);
    
    static ZydisStatus hookFormatPrintOperandSize(const ZydisFormatter* formatter, 
                                            char** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, 
                                            const ZydisDecodedOperand* operand, void* userData);
    
    static ZydisStatus hookFormatPrintSegment(const ZydisFormatter* formatter, 
                                          char** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, 
                                          const ZydisDecodedOperand* operand, void* userData);
    
    static ZydisStatus hookFormatPrintDecorator(const ZydisFormatter* formatter, 
                                          char** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, 
                                          const ZydisDecodedOperand* operand, void* userData);
    
    static ZydisStatus hookFormatPrintDisplacement(const ZydisFormatter* formatter, 
                                          char** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, 
                                          const ZydisDecodedOperand* operand, void* userData);
    
    static ZydisStatus hookFormatPrintImmediate(const ZydisFormatter* formatter, 
                                          char** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, 
                                          const ZydisDecodedOperand* operand, void* userData);
    
private:
    ZydisDecoder decoder_;
    ZydisFormatter formatter_;
    bool valid_;
    
    
    // Fills the branch info for an operand into the instruction info
    // returns true on success
    bool fillBranchInfo(ZydisDecodedOperand &operand, InstructionInfo &iinfo, InstructionInfo::BranchType bdranchType);
};

#endif // ARCHITECTUREX86_H

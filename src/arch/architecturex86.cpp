#include "architecturex86.h"
#include "log.h"
#include <QString>

Architecturex86::Architecturex86(Architecturex86::Mode mode)
{
    ZydisAddressWidth width;
    switch(mode)
    {
        case BIT16:
            ZydisDecoderInit(&decoder_, ZYDIS_MACHINE_MODE_REAL_16, ZYDIS_ADDRESS_WIDTH_16);
            break;
        case BIT32:
            ZydisDecoderInit(&decoder_, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_ADDRESS_WIDTH_32);
            break;
        case BIT64:
            ZydisDecoderInit(&decoder_, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);
            break;
        default:
            Log::error("Failed to intialize Zydis decoder: invalid mode");
            valid_ = false;
            return;
    }
    

    ZydisFormatterInit(&formatter_, ZYDIS_FORMATTER_STYLE_INTEL);
    
    /*
    ZydisFormatterSetHook(&formatter_, ZYDIS_FORMATTER_HOOK_FORMAT_OPERAND_REG, (const void**)&Architecturex86::hookFormatOperandReg);
    ZydisFormatterSetHook(&formatter_, ZYDIS_FORMATTER_HOOK_FORMAT_OPERAND_MEM, (const void**)&Architecturex86::hookFormatOperandMem);
    ZydisFormatterSetHook(&formatter_, ZYDIS_FORMATTER_HOOK_FORMAT_OPERAND_PTR, (const void**)&Architecturex86::hookFormatOperandPtr);
    ZydisFormatterSetHook(&formatter_, ZYDIS_FORMATTER_HOOK_FORMAT_OPERAND_IMM, (const void**)&Architecturex86::hookFormatOperandImm);
    ZydisFormatterSetHook(&formatter_, ZYDIS_FORMATTER_HOOK_PRINT_OPERANDSIZE, (const void**)&Architecturex86::hookFormatPrintOperandSize);
    ZydisFormatterSetHook(&formatter_, ZYDIS_FORMATTER_HOOK_PRINT_SEGMENT, (const void**)&Architecturex86::hookFormatPrintSegment);
    ZydisFormatterSetHook(&formatter_, ZYDIS_FORMATTER_HOOK_PRINT_DECORATOR, (const void**)&Architecturex86::hookFormatPrintDecorator);
    ZydisFormatterSetHook(&formatter_, ZYDIS_FORMATTER_HOOK_PRINT_DISPLACEMENT, (const void**)&Architecturex86::hookFormatPrintDisplacement);
    ZydisFormatterSetHook(&formatter_, ZYDIS_FORMATTER_HOOK_PRINT_IMMEDIATE, (const void**)&Architecturex86::hookFormatPrintImmediate);
    */
        
    valid_ = true;
}


bool Architecturex86::fillBranchInfo(ZydisDecodedOperand& operand, InstructionInfo& iinfo, InstructionInfo::BranchType branchType)
{
    if (branchType != InstructionInfo::BRANCH_TAKEN && branchType != InstructionInfo::BRANCH_NOTTAKEN)
    {
        return true;
    }
    switch (operand.type)
    {
        case ZYDIS_OPERAND_TYPE_MEMORY:
            // TODO: scale and index will matter in 16bit mode
            iinfo.setBranch(branchType, operand.mem.base);
            break;
        case ZYDIS_OPERAND_TYPE_POINTER:
            iinfo.setBranch(branchType, operand.ptr.offset);
            break;
        case ZYDIS_OPERAND_TYPE_REGISTER:
            InstructionInfo::BranchType type;
            
            if (branchType == InstructionInfo::BRANCH_ALWAYS)
            {
                branchType = static_cast<InstructionInfo::BranchType>(static_cast<int>(branchType) | InstructionInfo::BRANCH_ALWAYS_REG);
            }
            else if (branchType == InstructionInfo::BRANCH_NOTTAKEN)
            {
                branchType = static_cast<InstructionInfo::BranchType>(static_cast<int>(branchType) | InstructionInfo::BRANCH_NOTTAKEN_REG);
            }
            
            iinfo.setBranch(branchType, operand.reg.value);
            break;
        default:
            Log::error("Unsupported operand type for branch instruction");
            return false;
            break;
    }
    
    return true;
}



bool Architecturex86::instructionInfo(uint64_t instructionPointer, const char* data, std::size_t size, InstructionInfo& iinfo)
{
    ZydisDecodedInstruction instruction;
    if (ZYDIS_SUCCESS(ZydisDecoderDecodeBuffer(&decoder_, data, size, instructionPointer, &instruction)))
    {
        iinfo.setLength(instruction.length);
        
        switch (instruction.mnemonic)
        {
            case ZYDIS_MNEMONIC_JMP:
            {
                if (instruction.operandCount == 1)
                {
                    ZydisDecodedOperand &op = instruction.operands[0];
                    if (!fillBranchInfo(op, iinfo, InstructionInfo::BRANCH_ALWAYS))
                    {
                        return false;
                    }
                }
                break;
            }
        
            case ZYDIS_MNEMONIC_JB: // jc, jnae
            case ZYDIS_MNEMONIC_JBE: // jna
            case ZYDIS_MNEMONIC_JCXZ:
            case ZYDIS_MNEMONIC_JECXZ:
            case ZYDIS_MNEMONIC_JRCXZ:
            case ZYDIS_MNEMONIC_JKNZD:
            case ZYDIS_MNEMONIC_JKZD:
            case ZYDIS_MNEMONIC_JL: // jnge
            case ZYDIS_MNEMONIC_JLE: // jng
            case ZYDIS_MNEMONIC_JNB: // jae, jnc
            case ZYDIS_MNEMONIC_JNBE: // ja
            
            case ZYDIS_MNEMONIC_JNL: // jge
            case ZYDIS_MNEMONIC_JNLE: // jg
            
            case ZYDIS_MNEMONIC_JNO:
            case ZYDIS_MNEMONIC_JNP: // jpo
            case ZYDIS_MNEMONIC_JNS:
            
            case ZYDIS_MNEMONIC_JNZ: // jne
            case ZYDIS_MNEMONIC_JO:
            case ZYDIS_MNEMONIC_JP: // jpe
            case ZYDIS_MNEMONIC_JS:
            case ZYDIS_MNEMONIC_JZ:
            {
                if (instruction.operandCount == 1)
                {
                    ZydisDecodedOperand &op = instruction.operands[0];
                    if (!fillBranchInfo(op, iinfo, InstructionInfo::BRANCH_TAKEN))
                    {
                        return false;
                    }
                }
                break;
            }
        }
        
        return true;
    }

    return false;
}

bool Architecturex86::registerName(uint16_t id, std::string& name)
{
    const char *cname = ZydisRegisterGetString(id);
    if (cname == nullptr)
    {
        return false;
    }
    name.assign(cname);
    
    return true;
}


bool Architecturex86::instructionText(uint64_t instructionPointer, const char* data, std::size_t size, std::vector< Architecture::Token >& tokens)
{
    ZydisDecodedInstruction instruction;
    if (ZYDIS_SUCCESS(ZydisDecoderDecodeBuffer(&decoder_, data, size, instructionPointer, &instruction)))
    {
        char buffer[256];
        ZydisFormatterFormatInstruction(&formatter_, &instruction, buffer, sizeof(buffer));
        
        Architecture::Token token;
        token.type = Architecture::Token::TYPE_TEXT;
        token.text = std::string(buffer);
        
        tokens.push_back(std::move(token));
        
        return true;
    }
    
    return false;
}



ZydisStatus Architecturex86::hookFormatOperandImm(const ZydisFormatter* formatter, char ** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, const ZydisDecodedOperand* operand, void* userData)
{
    std::vector<Architecture::Token> *tokens = reinterpret_cast<std::vector<Architecture::Token>*>(userData);
    
    // operand->imm.
    
    return ZYDIS_STATUS_SUCCESS;
}


ZydisStatus Architecturex86::hookFormatOperandMem(const ZydisFormatter* formatter, char ** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, const ZydisDecodedOperand* operand, void* userData)
{
}


ZydisStatus Architecturex86::hookFormatOperandPtr(const ZydisFormatter* formatter, char ** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, const ZydisDecodedOperand* operand, void* userData)
{
}


ZydisStatus Architecturex86::hookFormatOperandReg(const ZydisFormatter* formatter, char ** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, const ZydisDecodedOperand* operand, void* userData)
{
}


ZydisStatus Architecturex86::hookFormatPrintDecorator(const ZydisFormatter* formatter, char ** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, const ZydisDecodedOperand* operand, void* userData)
{
}


ZydisStatus Architecturex86::hookFormatPrintDisplacement(const ZydisFormatter* formatter, char ** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, const ZydisDecodedOperand* operand, void* userData)
{
}


ZydisStatus Architecturex86::hookFormatPrintImmediate(const ZydisFormatter* formatter, char ** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, const ZydisDecodedOperand* operand, void* userData)
{
}


ZydisStatus Architecturex86::hookFormatPrintOperandSize(const ZydisFormatter* formatter, char ** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, const ZydisDecodedOperand* operand, void* userData)
{
}


ZydisStatus Architecturex86::hookFormatPrintSegment(const ZydisFormatter* formatter, char ** buffer, size_t bufferLen, const ZydisDecodedInstruction* instruction, const ZydisDecodedOperand* operand, void* userData)
{
}


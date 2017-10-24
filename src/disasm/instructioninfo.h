#ifndef INSTRUCTIONINFO_H
#define INSTRUCTIONINFO_H
#include <cstdint>

class InstructionInfo
{
public:
    InstructionInfo();
    
    inline uint32_t length()
    {
        return length_;
    }
    
    inline void setLength(uint32_t length)
    {
        length_ = length;
    }
    
    enum BranchType
    {
        BRANCH_TAKEN = 1,
        BRANCH_NOTTAKEN = 2,
        BRANCH_ALWAYS = 4, // the branch is always taken (e.g. jmp, call, ret)
        BRANCH_STOP = 8, // execution stops
        BRANCH_ALWAYS_REG = 16, // the branch location is a register
        BRANCH_TAKEN_REG = 16,
        BRANCH_NOTTAKEN_REG = 32,
    };
    
    
    // Adds branch info
    void setBranch(BranchType branch, uint64_t location);
    
private:
    uint32_t length_;
    uint8_t branchTypes_;
    
    // When branchTypes_ includes BRANCH_TAKEN or BRANCH_NOTTAKEB, branches[0] is the
    // branch that is taken while branch[1] is the branch not taken. If BRANCH_ALWAYS,
    // branches[0] is the branch location
    uint64_t branches_[2];
};

#endif // INSTRUCTIONINFO_H

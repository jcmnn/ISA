#include "instructioninfo.h"

InstructionInfo::InstructionInfo()
{

}


void InstructionInfo::setBranch(InstructionInfo::BranchType branch, uint64_t location)
{
    branchTypes_ |= branch;
    
    switch (branch)
    {
        case BRANCH_ALWAYS:
        case BRANCH_TAKEN:
            branches_[0] = location;
            break;
        case BRANCH_NOTTAKEN:
            branches_[1] = location;
            break;
        default:
            break;
    }
}

#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H


class SectionHandler;

class Disassembler
{
public:
    Disassembler(SectionHandler *sectionHandler);
    
private:
    SectionHandler *sectionHandler_;
};

#endif // DISASSEMBLER_H

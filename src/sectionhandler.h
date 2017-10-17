#ifndef SECTIONHANDLER_H
#define SECTIONHANDLER_H
#include "section.h"
#include <vector>

class SectionHandler
{
public:
    SectionHandler();

    void addSection(SectionPtr section);


private:
    std::vector<SectionPtr> sections_;
};

#endif // SECTIONHANDLER_H

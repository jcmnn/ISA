#include "sectionhandler.h"
#include "section.h"
#include <vector>

SectionHandler::SectionHandler()
{
}

void SectionHandler::addSection(SectionPtr section)
{
    sections_.push_back(section);
}

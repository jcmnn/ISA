#include "sectionhandler.h"
#include <vector>
#include "section.h"

SectionHandler::SectionHandler()
{

}

void SectionHandler::addSection(SectionPtr section)
{
    sections_.push_back(section);
}

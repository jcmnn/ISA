#include "projecthandler.h"
#include "isa.h"

ProjectHandler::ProjectHandler()
{

}

void ProjectHandler::open(CoffFilePtr pefile)
{
    MainWindow *window = ISA::get()->mainWindow();
    window->reset();
    window->updatePEInfo(pefile);
    
    std::vector<SectionPtr> &sections = pefile->sections();
    
    for (SectionPtr &section : sections)
    {
        ISA::get()->sectionHandler()->addSection(section);
    }
}

#ifndef PROJECTHANDLER_H
#define PROJECTHANDLER_H

#include "pe/cofffile.h"

class ProjectHandler
{
public:
    ProjectHandler();
    
    /* Open a new project using a CoffFile */
    void open(CoffFilePtr pefile);
};

#endif // PROJECTHANDLER_H

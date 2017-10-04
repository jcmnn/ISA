#include "isa.h"


ISA::ISA(int &argc, char *argv[]) : QApplication(argc, argv)
{
    mainWindow_.show();
}

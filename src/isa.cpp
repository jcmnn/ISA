#include "isa.h"
#include "log.h"
#include <QFile>

ISA *g_isa = nullptr;

ISA::ISA(int &argc, char *argv[]) : QApplication(argc, argv), disassembler_(&sectionHandler_)
{
    g_isa = this;
    QFile darkSheet(":/style/dark.qss");
    if (!darkSheet.open(QFile::ReadOnly))
    {
        Log::warning("Failed to load custom stylesheet");
    }
    else
    {
        Log::normal("Loading custom stylesheet");
        // setStyleSheet(QString(darkSheet.readAll()));
    }

    mainWindow_.show();

    Log::normal("Loaded ISA");
}



ISA *ISA::get()
{
    return g_isa;
}

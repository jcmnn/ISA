#ifndef ISA_H
#define ISA_H

#include "ui/mainwindow.h"
#include <QApplication>
#include "sectionhandler.h"
#include "projecthandler.h"
#include "disassembler.h"

class ISA : public QApplication
{
    Q_OBJECT
public:
    ISA (int &argc, char *argv[]);
    
    /* Returns a pointer to the global ISA object */
    static ISA *get();
    
    /* Returns a pointer to the section handler */
    inline SectionHandler *sectionHandler()
    {
        return &sectionHandler_;
    }
    
    /* Returns a pointer to the project handler */
    inline ProjectHandler *projectHandler()
    {
        return &projectHandler_;
    }
    
    /* Returns a pointer to the main window */
    inline MainWindow *mainWindow()
    {
        return &mainWindow_;
    }
    
    inline Disassembler *disassembler()
    {
        return &disassembler_;
    }

private:
    MainWindow mainWindow_;
    SectionHandler sectionHandler_;
    ProjectHandler projectHandler_;
    Disassembler disassembler_;
};

#endif // ISA_H

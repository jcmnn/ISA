#ifndef ISA_H
#define ISA_H

#include <QApplication>
#include "ui/mainwindow.h"

class ISA : public QApplication
{
public:
    ISA(int &argc, char *argv[]);

private:
    MainWindow mainWindow_;
};

#endif // ISA_H

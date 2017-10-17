#ifndef ISA_H
#define ISA_H

#include "ui/mainwindow.h"
#include <QApplication>

class ISA : public QApplication {
  Q_OBJECT
public:
  ISA(int &argc, char *argv[]);

private:
  MainWindow mainWindow_;
};

#endif // ISA_H

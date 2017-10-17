#include "isa.h"
#include "log.h"
#include <QFile>

ISA::ISA(int &argc, char *argv[]) : QApplication(argc, argv) {
  QFile darkSheet(":/style/dark.qss");
  if (!darkSheet.open(QFile::ReadOnly)) {
    Log::warning("Failed to load custom stylesheet");
  } else {
    Log::normal("Loading custom stylesheet");
    // setStyleSheet(QString(darkSheet.readAll()));
  }

  mainWindow_.show();

  Log::normal("Loaded ISA");
}

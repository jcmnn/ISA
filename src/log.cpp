#include "log.h"
#include "logmodel.h"
#include <iostream>

void Log::log(Log::MessageLevel level, const QString &message) {
  switch (level) {
  case Normal:
    std::cout << message.toStdString() << std::endl;
    break;
  case Warning:
  case Error:
    std::cerr << message.toStdString() << std::endl;
    break;
  }

  LogModel::get()->append(level, message);
}

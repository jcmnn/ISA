#ifndef LOG_H
#define LOG_H

#include <QMetaType>

class Log {
public:
  enum MessageLevel { Normal, Warning, Error };

  static void log(MessageLevel level, const QString &message);

  inline static void normal(const QString &message) { log(Normal, message); }

  inline static void warning(const QString &message) { log(Warning, message); }

  inline static void error(const QString &message) { log(Error, message); }
};

Q_DECLARE_METATYPE(Log::MessageLevel)

#endif // LOG_H

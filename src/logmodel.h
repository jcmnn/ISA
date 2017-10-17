#ifndef LOGMODEL_H
#define LOGMODEL_H

#include "log.h"
#include <QAbstractListModel>

class LogModel : public QAbstractListModel
{
    Q_OBJECT
public:
    /* Gets the global LogModel object */
    static LogModel *get();

    LogModel(QObject *parent = 0);

    void append(Log::MessageLevel level, const QString &text);

    QVariant data(const QModelIndex &index, int role) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    enum Roles
    {
        LevelRole = Qt::UserRole
    };

private:
    struct Entry
    {
        Log::MessageLevel level;
        QString text;
    };

    QList<Entry> entries_;
};

#endif // LOGMODEL_H

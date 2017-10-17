#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QAbstractItemModel>
#include <QPlainTextEdit>

class LogProxyModel;

class LogView : public QPlainTextEdit
{
    Q_OBJECT
public:
    LogView(QWidget *parent = 0);

private:
    LogProxyModel *model_;

public slots:
    void setModel(QAbstractItemModel *model);
    void rowsInserted(const QModelIndex &parent, int first, int last);
    void reset();
};

#endif // LOGVIEW_H

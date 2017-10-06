#include "logview.h"
#include <QIdentityProxyModel>
#include "logmodel.h"

#include <QApplication>

class LogProxyModel : public QIdentityProxyModel
{
public:
    LogProxyModel(QObject *parent = 0) : QIdentityProxyModel(parent)
    {

    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        switch(role)
        {
        case Qt::TextColorRole:
            switch (sourceModel()->data(index, LogModel::LevelRole).value<Log::MessageLevel>())
            {
            case Log::Normal:
                return QApplication::palette().text().color();
            case Log::Warning:
                return QColor(255, 156, 43);
            case Log::Error:
                return QColor(229, 15, 0);
            }
        default:
            return sourceModel()->data(index, role);
        }
    }
};


LogView::LogView(QWidget *parent) : QPlainTextEdit(parent), model_(new LogProxyModel(this))
{
    connect(model_, &QAbstractItemModel::rowsInserted, this, &LogView::rowsInserted);
    connect(model_, &QAbstractItemModel::modelReset, this, &LogView::reset);
}



void LogView::setModel(QAbstractItemModel *model)
{
    model_->setSourceModel(model);

    reset();
}



void LogView::rowsInserted(const QModelIndex &parent, int first, int last)
{
    for (int i = first; i <= last; ++i)
    {
        auto index = model_->index(i, 0, parent);

        QTextCharFormat format;

        auto foregroundColor = model_->data(index, Qt::TextColorRole);
        if (foregroundColor.isValid())
        {
            format.setForeground(foregroundColor.value<QColor>());
        }

        auto backgroundColor = model_->data(index, Qt::BackgroundColorRole);
        if (backgroundColor.isValid())
        {
            format.setBackground(backgroundColor.value<QColor>());
        }

        format.setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

        QString text = model_->data(index, Qt::DisplayRole).toString();

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        if (i != 0)
        {
            cursor.insertBlock();
        }

        cursor.insertText(text, format);
    }
}


void LogView::reset()
{
    document()->clear();
    if (model_)
    {
        rowsInserted(QModelIndex(), 0, model_->rowCount() - 1);
    }
}

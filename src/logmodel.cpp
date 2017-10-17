#include "logmodel.h"

LogModel *LogModel::get() {
  static LogModel model;
  return &model;
}

LogModel::LogModel(QObject *parent) : QAbstractListModel(parent) {}

void LogModel::append(Log::MessageLevel level, const QString &text) {
  beginInsertRows(QModelIndex(), entries_.size(), entries_.size());

  Entry entry;
  entry.level = level;
  entry.text = text;
  entries_.append(entry);

  endInsertRows();
}

QVariant LogModel::data(const QModelIndex &index, int role) const {
  int row = index.row();
  if (row < 0 || row >= entries_.size()) {
    return QVariant();
  }

  switch (role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    return entries_[row].text;
  case LevelRole:
    return entries_[row].level;
  default:
    return QVariant();
  }
}

int LogModel::rowCount(const QModelIndex &parent) const {
  return entries_.size();
}

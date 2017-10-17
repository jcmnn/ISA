#ifndef PEFILE_H
#define PEFILE_H

#include "section.h"
#include "cofffile.h"
#include <QAbstractTableModel>
#include <QIODevice>
#include <memory>

class PEFile : public CoffFile {
public:
  PEFile();

  bool parse(QIODevice *device);

};

typedef std::shared_ptr<PEFile> PEFilePtr;

class PEModel : public QAbstractTableModel {
public:
  PEModel(QObject *parent = Q_NULLPTR);
  void setFile(CoffFilePtr peFile);

protected:
  CoffFilePtr peFile_;
};

class PESectionModel : public PEModel {
public:
  PESectionModel(QObject *parent = Q_NULLPTR);

  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;

  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
};

class PECoffModel : public PEModel {
public:
  PECoffModel(QObject *parent = Q_NULLPTR);

  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;

  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
};

class PEOptionalModel : public PEModel {
public:
  PEOptionalModel(QObject *parent = Q_NULLPTR);

  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;

  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
};

#endif // PEFILE_H

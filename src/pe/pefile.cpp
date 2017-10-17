#include "pefile.h"
#include "log.h"
#include "pesection.h"
#include <QDataStream>
#include <time.h>


PEFile::PEFile()
{
}

bool PEFile::parse(QIODevice *device)
{
    QDataStream stream(device);

    stream.setByteOrder(QDataStream::LittleEndian);

    {
        char dosId[2];
        if (stream.readRawData(dosId, 2) != 2)
        {
            Log::error("PE file is too short");
            return false;
        }

        if (memcmp("MZ", dosId, 2) != 0)
        {
            Log::error("Invalid DOS header ID");
            return false;
        }
    }

    // We don't care about the DOS header so we'll skip it (unless we add DOS
    // support?) The offset to the PE section is at 0x3C
    device->seek(0x3C);

    if (stream.atEnd())
    {
        Log::error("Invalid PE file: stream ended");
        return false;
    }

    return CoffFile::parse(device);
}

const char *coffFields[] = {
    "Machine",         "NumberOfSections",
    "TimeDateStamp",   "PointerToSymbolTable",
    "NumberOfSymbols", "SizeOfOptionalHeader",
    "Characteristics",
};

static const int COFF_FIELD_COUNT = 7;

PECoffModel::PECoffModel(QObject *parent) : PEModel(parent)
{
}

PEModel::PEModel(QObject *parent) : QAbstractTableModel(parent)
{
}

void PEModel::setFile(CoffFilePtr peFile)
{
    beginResetModel();
    peFile_ = peFile;
    // emit dataChanged(index(0, 0), index(rowCount(), columnCount()));
    endResetModel();
}

int PECoffModel::rowCount(const QModelIndex &parent) const
{
    return COFF_FIELD_COUNT;
}

int PECoffModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant PECoffModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= COFF_FIELD_COUNT ||
        index.column() != 0 || !peFile_)
    {
        return QVariant();
    }

    switch (role)
    {
    case Qt::DisplayRole:
        switch (index.row())
        {
        case 0:
            return QStringLiteral("0x%1 (%2)")
                .arg(QString::number(peFile_->coffHeader_.machine, 16))
                .arg(CoffFile::machineString(static_cast<CoffFile::MachineType>(
                    peFile_->coffHeader_.machine)));
        case 1:
            return QString::number(peFile_->coffHeader_.numberOfSections);
        case 2:
        {
            time_t timeSinceEpoch =
                static_cast<time_t>(peFile_->coffHeader_.timeDateStamp);
            char *buf = asctime(gmtime(&timeSinceEpoch));
            return QStringLiteral("%1 (%2)")
                .arg(peFile_->coffHeader_.timeDateStamp)
                .arg(buf);
        }
        case 3:
            return QString::number(peFile_->coffHeader_.pointerToSymbolTable);
        case 4:
            return QString::number(peFile_->coffHeader_.numberOfSymbols);
        case 5:
            return QString::number(peFile_->coffHeader_.sizeOfOptionalHeader);
        case 6:
            return QStringLiteral("0x%1 (%2)")
                .arg(QString::number(peFile_->coffHeader_.characteristics, 16))
                .arg(CoffFile::coffCharString(
                    static_cast<CoffFile::CoffCharacteristics>(
                        peFile_->coffHeader_.characteristics)));
        }
    // fall through
    default:
        return QVariant();
    }
}

QVariant PECoffModel::headerData(int section, Qt::Orientation orientation,
                                 int role) const
{
    if (section < 0 || section >= COFF_FIELD_COUNT ||
        orientation != Qt::Vertical)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        return coffFields[section];
    }

    return QVariant();
}

const char *optionalFields[] = {
    "Signature",
    "MajorLinkerVersion",
    "MinorLinkerVersion",
    "SizeOfCode",
    "SizeOfInitializedData",
    "SizeOfUnitializedData",
    "AddressOfEntryPoint",
    "BaseOfCode",

    "BaseOfData",

    "ImageBase",
    "SectionAlignment",
    "FileAlignment",
    "MajorOperatingSystemVersion",
    "MinorOperatingSystemVersion",
    "MajorImageVersion",
    "MinorImageVersion",
    "MajorSubsystemVersion",
    "MinorSubsystemVersion",
    "Win32VersionValue",
    "SizeOfImage",
    "SizeOfHeaders",
    "Checksum",
    "Subsystem",
    "DLLCharacteristics",
    "SizeOfStackReserve",
    "SizeOfStackCommit",
    "SizeOfHeapReserve",
    "SizeOfHeapCommit",
    "LoaderFlags",
    "NumberOfRvaAndSizes",
};

static const int OPT_FIELD_COUNT = 30;

PEOptionalModel::PEOptionalModel(QObject *parent) : PEModel(parent)
{
}

int PEOptionalModel::rowCount(const QModelIndex &parent) const
{
    return OPT_FIELD_COUNT;
}

int PEOptionalModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant PEOptionalModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= OPT_FIELD_COUNT ||
        index.column() != 0 || !peFile_)
    {
        return QVariant();
    }

    switch (role)
    {
    case Qt::DisplayRole:
        switch (index.row())
        {
        case 0:
        { // signature
            QString s = QStringLiteral("0x%1 (%2)")
                            .arg(QString::number(
                                peFile_->optionalHeader_.signature, 16));
            switch (peFile_->optionalHeader_.signature)
            {
            case CoffFile::ET_PE32:
                return s.arg("PE32");
            case CoffFile::ET_ROM:
                return s.arg("ROM");
            case CoffFile::ET_PE32P:
                return s.arg("PE32+");
            default:
                return s.arg("Unknown");
            }
        }
        case 1: // majorLinkerVersion
            return QString::number(peFile_->optionalHeader_.majorLinkerVersion);
        case 2: // minorLinkerVersion
            return QString::number(peFile_->optionalHeader_.minorLinkerVersion);
        case 3: // sizeOfCode
            return QString::number(peFile_->optionalHeader_.sizeOfCode);
        case 4: // sizeOfInitializedData
            return QString::number(
                peFile_->optionalHeader_.sizeOfInitializedData);
        case 5: // sizeOfUnitializedData
            return QString::number(
                peFile_->optionalHeader_.sizeOfUninitialziedData);
        case 6: // addressOfEntryPoint
            return QStringLiteral("0x%1").arg(QString::number(
                peFile_->optionalHeader_.addressOfEntryPoint, 16));
        case 7: // baseOfCode
            return QStringLiteral("0x%1").arg(
                QString::number(peFile_->optionalHeader_.baseOfCode, 16));
        case 8: // baseOfData
            if (peFile_->optionalHeader_.signature == CoffFile::ET_PE32P)
            {
                return QVariant();
            }
            return QStringLiteral("0x%1").arg(
                QString::number(peFile_->optionalHeader_.baseOfData, 16));
        case 9: // imageBase
            return QStringLiteral("0x%1").arg(
                QString::number(peFile_->optionalHeader_.imageBase, 16));
        case 10: // sectionAlignment
            return QString::number(peFile_->optionalHeader_.sectionAlignment);
        case 11: // fileAlignment
            return QString::number(peFile_->optionalHeader_.fileAlignment);
        case 12: // majorOperatingSystemVersion
            return QString::number(
                peFile_->optionalHeader_.majorOperatingSystemVersion);
        case 13: // minorOperatingSystemVersion
            return QString::number(
                peFile_->optionalHeader_.minorOperatingSystemVersion);
        case 14: // majorImageVersion
            return QString::number(peFile_->optionalHeader_.majorImageVersion);
        case 15: // minorImageVersion
            return QString::number(peFile_->optionalHeader_.minorImageVersion);
        case 16: // majorSubsystemVersion
            return QString::number(
                peFile_->optionalHeader_.majorSubsystemVersion);
        case 17: // minorSubsystemVersion
            return QString::number(
                peFile_->optionalHeader_.minorSubsystemVersion);
        case 18: // win32VersionValue
            return QString::number(peFile_->optionalHeader_.win32VersionValue);
        case 19: // sizeOfImage
            return QString::number(peFile_->optionalHeader_.sizeOfImage);
        case 20: // sizeOfHeaders
            return QString::number(peFile_->optionalHeader_.sizeOfHeaders);
        case 21: // checksum
            return QString::number(peFile_->optionalHeader_.checkSum);
        case 22: // subsystem
            return QStringLiteral("%1 (%2)")
                .arg(peFile_->optionalHeader_.subsystem)
                .arg(CoffFile::subsystemString(static_cast<CoffFile::Subsystem>(
                    peFile_->optionalHeader_.subsystem)));
        case 23: // dllCharacteristics
            return QStringLiteral("0x%1 (%2)")
                .arg(QString::number(
                    peFile_->optionalHeader_.dllCharacteristics, 16))
                .arg(CoffFile::dllCharString(
                    static_cast<CoffFile::DllCharacteristics>(
                        peFile_->optionalHeader_.dllCharacteristics)));
        case 24: // sizeOfStackReserve
            return QString::number(peFile_->optionalHeader_.sizeOfStackReserve);
        case 25: // sizeOfStackCommit
            return QString::number(peFile_->optionalHeader_.sizeOfStackCommit);
        case 26: // sizeOfHeapReserve
            return QString::number(peFile_->optionalHeader_.sizeOfHeapReserve);
        case 27: // sizeOfHeapCommit
            return QString::number(peFile_->optionalHeader_.sizeOfHeapCommit);
        case 28: // loaderFlags
            return QStringLiteral("0x%1").arg(
                QString::number(peFile_->optionalHeader_.loaderFlags, 16));
        case 29: // numberOfRvaAndSizes
            return QString::number(
                peFile_->optionalHeader_.numberOfRvaAndSizes);
        }

    // fall through
    default:
        return QVariant();
    }
}

QVariant PEOptionalModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const
{
    if (section < 0 || section >= OPT_FIELD_COUNT ||
        orientation != Qt::Vertical)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        return optionalFields[section];
    }

    return QVariant();
}

const char *sectionFields[] = {
    "Name",
    "VirtualSize",
    "VirtualAddress",
    "SizeOfRawData",
    "PointerToRawData",
    "PointerToRelocations",
    "PointerToLineNumbers",
    "NumberOfRelocations",
    "NumberOfLineNumbers",
    "Characteristics",
};

static const int SECTION_FIELDS_COUNT = 10;

PESectionModel::PESectionModel(QObject *parent) : PEModel(parent)
{
}

int PESectionModel::rowCount(const QModelIndex &parent) const
{
    if (peFile_)
        return peFile_->sectionTable_.size();
    else
        return 0;
}

int PESectionModel::columnCount(const QModelIndex &parent) const
{
    return SECTION_FIELDS_COUNT;
}

QVariant PESectionModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= peFile_->sectionTable_.size() ||
        index.column() < 0 || index.column() >= SECTION_FIELDS_COUNT ||
        !peFile_)
    {
        return QVariant();
    }

    switch (role)
    {
    case Qt::DisplayRole:
        switch (index.column())
        {
        case 0: // name
            return peFile_->sectionTable_[index.row()].name;
        case 1: // virtualSize
            return QString::number(
                peFile_->sectionTable_[index.row()].virtualSize);
        case 2: // virtualAddress
            return QStringLiteral("0x%1").arg(QString::number(
                peFile_->sectionTable_[index.row()].virtualAddress, 16));
        case 3: // sizeOfRawData
            return QString::number(
                peFile_->sectionTable_[index.row()].sizeOfRawData);
        case 4: // pointerToRawData
            return QStringLiteral("0x%1").arg(QString::number(
                peFile_->sectionTable_[index.row()].pointerToRawData, 16));
        case 5: // pointerToRelocations
            return QStringLiteral("0x%1").arg(QString::number(
                peFile_->sectionTable_[index.row()].pointerToRelocations, 16));
        case 6: // pointerToLineNumbers
            return QStringLiteral("0x%1").arg(QString::number(
                peFile_->sectionTable_[index.row()].pointerToLineNumbers, 16));
        case 7: // numberOfRelocations
            return QString::number(
                peFile_->sectionTable_[index.row()].numberOfRelocations);
        case 8: // numberOfLineNumbers
            return QString::number(
                peFile_->sectionTable_[index.row()].numberOfLineNumbers);
        case 9: // characteristics
            return QStringLiteral("0x%1 (%2)")
                .arg(QString::number(
                    peFile_->sectionTable_[index.row()].characteristics, 16))
                .arg(CoffFile::sectionCharString(
                    static_cast<CoffFile::SectionCharacteristics>(
                        peFile_->sectionTable_[index.row()].characteristics)));
        }
    // fall through
    default:
        return QVariant();
    }
}

QVariant PESectionModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
{
    if (section < 0 || section >= SECTION_FIELDS_COUNT ||
        orientation != Qt::Horizontal)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        return sectionFields[section];
    }

    return QVariant();
}

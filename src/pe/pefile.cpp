#include "pefile.h"
#include <QDataStream>
#include "log.h"
#include <time.h>
#include "pesection.h"

// These constants include the optional header extension
static const uint32_t OPTHEADER_SIZE_BASE_PE32 = 96;
static const uint32_t OPTHEADER_SIZE_BASE_PE32P = 112;

PEFile::PEFile(QIODevice *device) : device_(device), valid_(false)
{
    QDataStream stream(device);

    stream.setByteOrder(QDataStream::LittleEndian);

    {
        char dosId[2];
        if (stream.readRawData(dosId, 2) != 2)
        {
            Log::error("PE file is too short");
            return;
        }

        if (memcmp("MZ", dosId, 2) != 0)
        {
            Log::error("Invalid DOS header ID");
            return;
        }
    }

    // We don't care about the DOS header so we'll skip it (unless we add DOS support?)
    // The offset to the PE section is at 0x3C
    device->seek(0x3C);

    uint32_t peOffset;
    stream >> peOffset;

    device->seek(peOffset);

    if (stream.atEnd())
    {
        Log::error("Invalid PE file: stream ended");
        return;
    }

    // PE ID
    {
        uint32_t peId;
        stream >> peId;

        if (peId != 0x4550) // PE\0\0
        {
            Log::error("Invalid PE header ID");
            return;
        }
    }


    if (!parseCoffHeader(stream))
    {
        return;
    }

    optionalHeaderExists_ = (coffHeader_.sizeOfOptionalHeader != 0);

    if (optionalHeaderExists_)
    {
        if (!parseOptionalHeader(stream))
        {
            return;
        }
    }

    if (!parseSectionTable(stream))
    {
        return;
    }

    valid_ = true;
}


bool PEFile::parseCoffHeader(QDataStream &stream)
{
    stream >> coffHeader_.machine;
    stream >> coffHeader_.numberOfSections;
    stream >> coffHeader_.timeDateStamp;
    stream >> coffHeader_.pointerToSymbolTable;
    stream >> coffHeader_.numberOfSymbols;
    stream >> coffHeader_.sizeOfOptionalHeader;
    stream >> coffHeader_.characteristics;

    if (stream.atEnd())
    {
        Log::error("Invalid PE file: stream ended");
        return false;
    }

    return true;
}


bool PEFile::parseOptionalHeader(QDataStream &stream)
{
    stream >> optionalHeader_.signature;
    if (stream.atEnd())
    {
        Log::error("Invalid PE file: stream ended");
        return false;
    }

    bool pe32 = false;

    switch (optionalHeader_.signature)
    {
    case ET_PE32:
        if (coffHeader_.sizeOfOptionalHeader < OPTHEADER_SIZE_BASE_PE32)
        {
            Log::error("Invalid PE file: optional header is too short for PE32");
            return false;
        }
        pe32 = true;
        break;
    case ET_PE32P:
        if (coffHeader_.sizeOfOptionalHeader < OPTHEADER_SIZE_BASE_PE32P)
        {
            Log::error("Invalid PE fil: optional header is too short for PE32+");
            return false;
        }
        break;
    case ET_ROM:
        Log::error("ROM files are not supported at this time");
        return false;
    default:
        Log::error("Invalid PE file: unknown optional header signature");
        return false;
    }

    stream >> optionalHeader_.majorLinkerVersion;
    stream >> optionalHeader_.minorLinkerVersion;
    stream >> optionalHeader_.sizeOfCode;
    stream >> optionalHeader_.sizeOfInitializedData;
    stream >> optionalHeader_.sizeOfUninitialziedData;
    stream >> optionalHeader_.addressOfEntryPoint;
    stream >> optionalHeader_.baseOfCode;

    // A temporary variable for reading 32 bit fields for the pe32 format.
    // We cannot read directly to the structure because some fields are 64 bit to accomodate for PE32+
    uint32_t tempPe32;

    if (pe32)
    {
        stream >> optionalHeader_.baseOfData;

        // Start optional header extension
        stream >> tempPe32;
        optionalHeader_.imageBase = tempPe32;
    }
    else
    {
        stream >> optionalHeader_.imageBase;
    }

    stream >> optionalHeader_.sectionAlignment;
    stream >> optionalHeader_.fileAlignment;
    stream >> optionalHeader_.majorOperatingSystemVersion;
    stream >> optionalHeader_.minorOperatingSystemVersion;
    stream >> optionalHeader_.majorImageVersion;
    stream >> optionalHeader_.minorImageVersion;
    stream >> optionalHeader_.majorSubsystemVersion;
    stream >> optionalHeader_.minorSubsystemVersion;
    stream >> optionalHeader_.win32VersionValue;
    stream >> optionalHeader_.sizeOfImage;
    stream >> optionalHeader_.sizeOfHeaders;
    stream >> optionalHeader_.checkSum;
    stream >> optionalHeader_.subsystem;
    stream >> optionalHeader_.dllCharacteristics;
    if (pe32)
    {
        stream >> tempPe32;
        optionalHeader_.sizeOfStackReserve = tempPe32;
        stream >> tempPe32;
        optionalHeader_.sizeOfStackCommit = tempPe32;
        stream >> tempPe32;
        optionalHeader_.sizeOfHeapReserve = tempPe32;
        stream >> tempPe32;
        optionalHeader_.sizeOfHeapCommit = tempPe32;
    }
    else
    {
        stream >> optionalHeader_.sizeOfStackReserve;
        stream >> optionalHeader_.sizeOfStackCommit;
        stream >> optionalHeader_.sizeOfHeapReserve;
        stream >> optionalHeader_.sizeOfHeapCommit;
    }
    stream >> optionalHeader_.loaderFlags;
    stream >> optionalHeader_.numberOfRvaAndSizes;

    if (stream.atEnd())
    {
        Log::error("Invalid PE file: stream ended");
        return false;
    }

    uint16_t sizeLeft;
    if (pe32)
    {
        sizeLeft = coffHeader_.sizeOfOptionalHeader - OPTHEADER_SIZE_BASE_PE32;
    }
    else
    {
        sizeLeft = coffHeader_.sizeOfOptionalHeader - OPTHEADER_SIZE_BASE_PE32P;
    }

    uint16_t sizeNeeded = optionalHeader_.numberOfRvaAndSizes * 8; // 2 dwords
    if (sizeLeft < sizeNeeded)
    {
        Log::error("Invalid PE file: data-directory entries exceed optional header size");
        return false;
    }

    for (uint32_t i = 0; i < optionalHeader_.numberOfRvaAndSizes; ++i)
    {
        DataDirectory dataDirectory;
        stream >> dataDirectory.virtualAddress;
        stream >> dataDirectory.size;
        dataDirectories_.push_back(dataDirectory);
    }

    sizeLeft -= sizeNeeded;
    if (sizeLeft != 0)
    {
        Log::warning(QString("There are %1 excess bytes in the optional header; skipping over them").arg(sizeLeft));
        stream.skipRawData(sizeLeft);
    }

    if (stream.atEnd())
    {
        Log::error("Invalid PE file: stream ended");
        return false;
    }

    return true;
}

bool PEFile::parseSectionTable(QDataStream &stream)
{
    for (uint16_t i = 0; i < coffHeader_.numberOfSections; ++i)
    {
        SectionHeader header;
        header.name[8] = '\0';
        if (stream.readRawData(header.name, 8) != 8)
        {
            Log::error("Invalid PE file: stream ended");
            return false;
        }
        stream >> header.virtualSize;
        stream >> header.virtualAddress;
        stream >> header.sizeOfRawData;
        stream >> header.pointerToRawData;
        stream >> header.pointerToRelocations;
        stream >> header.pointerToLineNumbers;
        stream >> header.numberOfRelocations;
        stream >> header.numberOfLineNumbers;
        stream >> header.characteristics;

        sectionTable_.push_back(header);
    }

    if (stream.atEnd())
    {
        Log::error("Invalid PE file: stream ended");
        return false;
    }

    return true;
}

bool PEFile::parseSections(QDataStream &stream)
{
    for (const SectionHeader &header : sectionTable_)
    {
        PESectionPtr section = std::make_shared<PESection>(header);

        std::vector<char> data(header.sizeOfRawData);
        device_->seek(header.pointerToRawData);
        if (stream.readRawData(data.data(), header.sizeOfRawData) != header.sizeOfRawData)
        {
            Log::error("Invalid PE file: stream ended reading section data");
            return false;
        }

        section->setRawData(std::move(data));

        sections_.push_back(std::static_pointer_cast<Section>(section));
    }

    return true;
}

const QString PEFile::machineString(PEFile::MachineType type)
{
    switch(type)
    {
    case MACH_UNKNOWN:
        return "Unknown";
    case MACH_AM33:
        return "Matsushita AM33";
    case MACH_AMD64:
        return "amd64";
    case MACH_ARM:
        return "ARM little endian";
    case MACH_ARM64:
        return "ARM64 little endian";
    case MACH_ARMNT:
        return "ARM Thumb-2 little endian";
    case MACH_EBC:
        return "EFI byte code";
    case MACH_I386:
        return "Intel 386";
    case MACH_IA64:
        return "Intel Itanium";
    case MACH_M32R:
        return "Mitsubishi M32R little endian";
    case MACH_MIPS16:
        return "MIPS16";
    case MACH_MIPSFPU:
        return "MIPS with FPU";
    case MACH_MIPSFPU16:
        return "MIPS16 with FPU";
    case MACH_POWERPC:
        return "Power PC little endian";
    case MACH_POWERPCFP:
        return "Power PC w/ floating point support";
    case MACH_R4000:
        return "MIPS little endian";
    case MACH_RISCV32:
        return "RISC-V 32-bit";
    case MACH_RISCV64:
        return "RISC-V 64-bit";
    case MACH_RISCV128:
        return "RISC-V 128-bit";
    case MACH_SH3:
        return "Hitachi SH3";
    case MACH_SH3DSP:
        return "Hitachi SH3 DSP";
    case MACH_SH4:
        return "Hitachi SH4";
    case MACH_SH5:
        return "Hitachi SH5";
    case MACH_THUMB:
        return "Thumb";
    case MACH_WCEMIPSV2:
        return "MIPS little-endian WCE v3";
    default:
        return "Unknown";
    }
}

struct CCharString
{
    PEFile::CoffCharacteristics characteristic;
    char *string;
};

CCharString coffCharStrings[] = {
    {PEFile::CCHAR_RELOCS_STRIPPED, "RELOCS_STRIPPED"},
    {PEFile::CCHAR_EXECUTABLE_IMAGE, "EXECUTABLE_IMAGE"},
    {PEFile::CCHAR_LINE_NUMS_STRIPPED, "LINE_NUMS_STRIPPED"},
    {PEFile::CCHAR_LOCAL_SYMS_STRIPPED, "LOCAL_SYMS_STRIPPED"},
    {PEFile::CCHAR_AGRESSIVE_WS_TRIM, "AGRESSIVE_WS_TRIM"},
    {PEFile::CCHAR_LARGE_ADDRESS_AWARE, "LARGE_ADDRESS_AWARE"},
    {PEFile::CCHAR_BYTES_RESERVED_LO, "BYTES_RESERVED_LO"},
    {PEFile::CCHAR_MACHINE_32BIT, "32BIT_MACHINE"},
    {PEFile::CCHAR_DEBUG_STRIPPED, "DEBUG_STRIPPED"},
    {PEFile::CCHAR_REMOVABLE_RUN_FROM_SWAP, "REMOVABLE_RUN_FROM_SWAP"},
    {PEFile::CCHAR_NET_RUN_FROM_SWAP, "NET_RUN_FROM_SWAP"},
    {PEFile::CCHAR_SYSTEM, "SYSTEM"},
    {PEFile::CCHAR_DLL, "DLL"},
    {PEFile::CCHAR_UP_SYSTEM_ONLY, "UP_SYSTEM_ONLY"},
    {PEFile::CCHAR_BYTES_RESERVED_HI, "BYTES_RESERVED_HI"},
};

static const int COFF_CHAR_STRINGS_COUNT = 15;

const QString PEFile::coffCharString(PEFile::CoffCharacteristics characteristics)
{
    QString string;

    for (int i = 0; i < COFF_CHAR_STRINGS_COUNT; ++i)
    {
        if (characteristics & coffCharStrings[i].characteristic)
        {
            if (!string.isEmpty())
            {
                string += " | ";
            }
            string += coffCharStrings[i].string;
        }
    }

    return string;
}

const QString PEFile::subsystemString(PEFile::Subsystem subsystem)
{
    switch(subsystem)
    {
    case SUBSYS_UNKNOWN:
        return "Unknown";
    case SUBSYS_NATIVE:
        return "Native";
    case SUBSYS_WINDOWS_GUI:
        return "Windows GUI";
    case SUBSYS_WINDOWS_CUI:
        return "Windows character subsystem";
    case SUBSYS_OS2_CUI:
        return "OS/2 character subsystem";
    case SUBSYS_POSIX_CUI:
        return "Posix character subsystem";
    case SUBSYS_NATIVE_WINDOWS:
        return "Native Win9x driver";
    case SUBSYS_WINDOWS_CE_GUI:
        return "Windows CE";
    case SUBSYS_EFI_APPLICATION:
        return "EFI application";
    case SUBSYS_EFI_BOOT_SERVICE_DRIVER:
        return "EFI boot service driver";
    case SUBSYS_EFI_RUNTIME_DRIVER:
        return "EFI runtime driver";
    case SUBSYS_EFI_ROM:
        return "EFI ROM image";
    case SUBSYS_XBOX:
        return "XBOX";
    case SUBSYS_WINDOWS_BOOT_APPLICATION:
        return "Windows boot application";
    default:
        return "Unknown";
    }
}

struct DCharString
{
    PEFile::DllCharacteristics characteristic;
    char *string;
};

DCharString dllCharStrings[] = {
    {PEFile::DCHAR_HIGHENTROPY_VA, "HIGHENTROPY_VA"},
    {PEFile::DCHAR_DYNAMIC_BASE, "DYNAMIC_BASE"},
    {PEFile::DCHAR_FORCE_INTEGRITY, "FORCE_INTEGRITY"},
    {PEFile::DCHAR_NX_COMPAT, "NX_COMPAT"},
    {PEFile::DCHAR_NO_ISOLATION, "NO_ISOLATION"},
    {PEFile::DCHAR_NO_SEH, "NO_SEH"},
    {PEFile::DCHAR_NO_BIND, "NO_BIND"},
    {PEFile::DCHAR_APPCONTAINER, "APPCONTAINER"},
    {PEFile::DCHAR_WDM_DRIVER, "WDM_DRIVER"},
    {PEFile::DCHAR_GUARD_CF, "GUARD_CF"},
    {PEFile::DCHAR_TERMINAL_SERVER_AWARE, "TERMINAL_SERVER_AWARE"},
};

static const int DLL_CHAR_STRINGS_COUNT = 11;

const QString PEFile::dllCharString(PEFile::DllCharacteristics characteristics)
{
    QString string;

    for (int i = 0; i < DLL_CHAR_STRINGS_COUNT; ++i)
    {
        if (characteristics & dllCharStrings[i].characteristic)
        {
            if (!string.isEmpty())
            {
                string += " | ";
            }
            string += dllCharStrings[i].string;
        }
    }

    return string;
}

struct SCharString
{
    PEFile::SectionCharacteristics characteristic;
    char *string;
};

SCharString sectionCharStrings[] = {
    {PEFile::SCHAR_TYPE_NO_PAD, "TYPE_NO_PAD"},
    {PEFile::SCHAR_CNT_CODE, "CNT_CODE"},
    {PEFile::SCHAR_CNT_INITIALIZED_DATA, "CNT_INITIALIZED_DATA"},
    {PEFile::SCHAR_CNT_UNINITIALIZED_DATA, "CNT_UNINITIALIZED_DATA"},
    {PEFile::SCHAR_LNK_OTHER, "LNK_OTHER"},
    {PEFile::SCHAR_LNK_INFO, "LNK_INFO"},
    {PEFile::SCHAR_LNK_REMOVE, "LNK_REMOVE"},
    {PEFile::SCHAR_LNK_COMDAT, "LNK_COMDAT"},
    {PEFile::SCHAR_GPREL, "GPREL"},
    {PEFile::SCHAR_MEM_PURGEABLE, "MEM_PURGABLE"},
    {PEFile::SCHAR_MEM_LOCKED, "MEM_LOCKED"},
    {PEFile::SCHAR_MEM_PRELOAD, "MEM_PRELOAD"},
    {PEFile::SCHAR_ALGIN_1BYTES, "ALIGN_1BYTES"},
    {PEFile::SCHAR_ALIGN_2BYTES, "ALIGN_2BYTES"},
    {PEFile::SCHAR_ALIGN_4BYTES, "ALIGN_4BYTES"},
    {PEFile::SCHAR_ALIGN_8BYTES, "ALIGN_8BYTES"},
    {PEFile::SCHAR_ALIGN_16BYTES, "ALIGN_16BYTES"},
    {PEFile::SCHAR_ALIGN_32BYTES, "ALIGN_32BYTES"},
    {PEFile::SCHAR_ALIGN_64BYTES, "ALIGN_64BYTES"},
    {PEFile::SCHAR_ALIGN_128BYTES, "ALIGN_128BYTES"},
    {PEFile::SCHAR_ALIGN_256BYTES, "ALIGN_256BYTES"},
    {PEFile::SCHAR_ALIGN_512BYTES, "ALIGN_512BYTES"},
    {PEFile::SCHAR_ALIGN_1024BYTES, "ALIGN_1024BYTES"},
    {PEFile::SCHAR_ALIGN_2048BYTES, "ALIGN_2048BYTES"},
    {PEFile::SCHAR_ALIGN_4096BYTES, "ALIGN_4096BYTES"},
    {PEFile::SCHAR_ALIGN_8192BYTES, "ALIGN_8192BYTES"},
    {PEFile::SCHAR_LNK_NRELOC_OVFL, "LNK_NRELOC_OVFL"},
    {PEFile::SCHAR_MEM_DISCARDABLE, "MEM_DISCARDABLE"},
    {PEFile::SCHAR_MEM_NOT_CACHED, "MEM_NOT_CACHED"},
    {PEFile::SCHAR_MEM_NOT_PAGED, "MEM_NOT_PAGED"},
    {PEFile::SCHAR_MEM_SHARED, "MEM_SHARED"},
    {PEFile::SCHAR_MEM_EXECUTE, "MEM_EXECUTE"},
    {PEFile::SCHAR_MEM_READ, "MEM_READ"},
    {PEFile::SCHAR_MEM_WRITE, "MEM_WRITE"},
};

static const int SECTION_CHAR_STRINGS_COUNT = 34;

const QString PEFile::sectionCharString(PEFile::SectionCharacteristics characteristics)
{
    QString string;

    for (int i = 0; i < SECTION_CHAR_STRINGS_COUNT; ++i)
    {
        if (characteristics & sectionCharStrings[i].characteristic)
        {
            if (!string.isEmpty())
            {
                string += " | ";
            }
            string += sectionCharStrings[i].string;
        }
    }

    return string;
}

char *coffFields[] = {
    "Machine",
    "NumberOfSections",
    "TimeDateStamp",
    "PointerToSymbolTable",
    "NumberOfSymbols",
    "SizeOfOptionalHeader",
    "Characteristics",
};

static const int COFF_FIELD_COUNT = 7;

PECoffModel::PECoffModel(QObject *parent) : PEModel(parent)
{

}

PEModel::PEModel(QObject *parent) : QAbstractTableModel(parent)
{

}

void PEModel::setFile(PEFilePtr peFile)
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
    if (index.row() < 0 || index.row() >= COFF_FIELD_COUNT || index.column() != 0 || !peFile_)
    {
        return QVariant();
    }

    switch(role)
    {
    case Qt::DisplayRole:
        switch(index.row())
        {
        case 0:
            return QStringLiteral("0x%1 (%2)").arg(QString::number(peFile_->coffHeader_.machine, 16)).arg(PEFile::machineString(static_cast<PEFile::MachineType>(peFile_->coffHeader_.machine)));
        case 1:
            return QString::number(peFile_->coffHeader_.numberOfSections);
        case 2:
        {
            time_t timeSinceEpoch = static_cast<time_t>(peFile_->coffHeader_.timeDateStamp);
            char buf[26];
            errno_t err = asctime_s(buf, gmtime(&timeSinceEpoch));
            return QStringLiteral("%1 (%2)").arg(peFile_->coffHeader_.timeDateStamp).arg(buf);
        }
        case 3:
            return QString::number(peFile_->coffHeader_.pointerToSymbolTable);
        case 4:
            return QString::number(peFile_->coffHeader_.numberOfSymbols);
        case 5:
            return QString::number(peFile_->coffHeader_.sizeOfOptionalHeader);
        case 6:
            return QStringLiteral("0x%1 (%2)").arg(QString::number(peFile_->coffHeader_.characteristics, 16)).arg(PEFile::coffCharString(static_cast<PEFile::CoffCharacteristics>(peFile_->coffHeader_.characteristics)));
        }
        // fall through
    default:
        return QVariant();
    }
}

QVariant PECoffModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section >= COFF_FIELD_COUNT || orientation != Qt::Vertical)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        return coffFields[section];
    }

    return QVariant();
}


char *optionalFields[] = {
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
    if (index.row() < 0 || index.row() >= OPT_FIELD_COUNT || index.column() != 0 || !peFile_)
    {
        return QVariant();
    }

    switch(role)
    {
    case Qt::DisplayRole:
        switch(index.row())
        {
        case 0: // signature
        {
            QString s = QStringLiteral("0x%1 (%2)").arg(QString::number(peFile_->optionalHeader_.signature, 16));
            switch (peFile_->optionalHeader_.signature)
            {
            case PEFile::ET_PE32:
                return s.arg("PE32");
            case PEFile::ET_ROM:
                return s.arg("ROM");
            case PEFile::ET_PE32P:
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
            return QString::number(peFile_->optionalHeader_.sizeOfInitializedData);
        case 5: // sizeOfUnitializedData
            return QString::number(peFile_->optionalHeader_.sizeOfUninitialziedData);
        case 6: // addressOfEntryPoint
            return QStringLiteral("0x%1").arg(QString::number(peFile_->optionalHeader_.addressOfEntryPoint, 16));
        case 7: // baseOfCode
            return QStringLiteral("0x%1").arg(QString::number(peFile_->optionalHeader_.baseOfCode, 16));
        case 8: // baseOfData
            if (peFile_->optionalHeader_.signature == PEFile::ET_PE32P)
            {
                return QVariant();
            }
            return QStringLiteral("0x%1").arg(QString::number(peFile_->optionalHeader_.baseOfData, 16));
        case 9: // imageBase
            return QStringLiteral("0x%1").arg(QString::number(peFile_->optionalHeader_.imageBase, 16));
        case 10: // sectionAlignment
            return QString::number(peFile_->optionalHeader_.sectionAlignment);
        case 11: // fileAlignment
            return QString::number(peFile_->optionalHeader_.fileAlignment);
        case 12: // majorOperatingSystemVersion
            return QString::number(peFile_->optionalHeader_.majorOperatingSystemVersion);
        case 13: // minorOperatingSystemVersion
            return QString::number(peFile_->optionalHeader_.minorOperatingSystemVersion);
        case 14: // majorImageVersion
            return QString::number(peFile_->optionalHeader_.majorImageVersion);
        case 15: // minorImageVersion
            return QString::number(peFile_->optionalHeader_.minorImageVersion);
        case 16: // majorSubsystemVersion
            return QString::number(peFile_->optionalHeader_.majorSubsystemVersion);
        case 17: // minorSubsystemVersion
            return QString::number(peFile_->optionalHeader_.minorSubsystemVersion);
        case 18: // win32VersionValue
            return QString::number(peFile_->optionalHeader_.win32VersionValue);
        case 19: // sizeOfImage
            return QString::number(peFile_->optionalHeader_.sizeOfImage);
        case 20: // sizeOfHeaders
            return QString::number(peFile_->optionalHeader_.sizeOfHeaders);
        case 21: // checksum
            return QString::number(peFile_->optionalHeader_.checkSum);
        case 22: // subsystem
            return QStringLiteral("%1 (%2)").arg(peFile_->optionalHeader_.subsystem).arg(PEFile::subsystemString(static_cast<PEFile::Subsystem>(peFile_->optionalHeader_.subsystem)));
        case 23: // dllCharacteristics
            return QStringLiteral("0x%1 (%2)").arg(QString::number(peFile_->optionalHeader_.dllCharacteristics, 16)).arg(PEFile::dllCharString(static_cast<PEFile::DllCharacteristics>(peFile_->optionalHeader_.dllCharacteristics)));
        case 24: // sizeOfStackReserve
            return QString::number(peFile_->optionalHeader_.sizeOfStackReserve);
        case 25: // sizeOfStackCommit
            return QString::number(peFile_->optionalHeader_.sizeOfStackCommit);
        case 26: // sizeOfHeapReserve
            return QString::number(peFile_->optionalHeader_.sizeOfHeapReserve);
        case 27: // sizeOfHeapCommit
            return QString::number(peFile_->optionalHeader_.sizeOfHeapCommit);
        case 28: // loaderFlags
            return QStringLiteral("0x%1").arg(QString::number(peFile_->optionalHeader_.loaderFlags, 16));
        case 29: // numberOfRvaAndSizes
            return QString::number(peFile_->optionalHeader_.numberOfRvaAndSizes);
        }

        // fall through
    default:
        return QVariant();
    }
}

QVariant PEOptionalModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section >= OPT_FIELD_COUNT || orientation != Qt::Vertical)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        return optionalFields[section];
    }

    return QVariant();
}

char *sectionFields[] = {
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
    if (index.row() < 0 || index.row() >= peFile_->sectionTable_.size() || index.column() < 0 || index.column() >= SECTION_FIELDS_COUNT || !peFile_)
    {
        return QVariant();
    }

    switch(role)
    {
    case Qt::DisplayRole:
        switch(index.column())
        {
        case 0: // name
            return peFile_->sectionTable_[index.row()].name;
        case 1: // virtualSize
            return QString::number(peFile_->sectionTable_[index.row()].virtualSize);
        case 2: // virtualAddress
            return QStringLiteral("0x%1").arg(QString::number(peFile_->sectionTable_[index.row()].virtualAddress, 16));
        case 3: // sizeOfRawData
            return QString::number(peFile_->sectionTable_[index.row()].sizeOfRawData);
        case 4: // pointerToRawData
            return QStringLiteral("0x%1").arg(QString::number(peFile_->sectionTable_[index.row()].pointerToRawData, 16));
        case 5: // pointerToRelocations
            return QStringLiteral("0x%1").arg(QString::number(peFile_->sectionTable_[index.row()].pointerToRelocations, 16));
        case 6: // pointerToLineNumbers
            return QStringLiteral("0x%1").arg(QString::number(peFile_->sectionTable_[index.row()].pointerToLineNumbers, 16));
        case 7: // numberOfRelocations
            return QString::number(peFile_->sectionTable_[index.row()].numberOfRelocations);
        case 8: // numberOfLineNumbers
            return QString::number(peFile_->sectionTable_[index.row()].numberOfLineNumbers);
        case 9: // characteristics
            return QStringLiteral("0x%1 (%2)").arg(QString::number(peFile_->sectionTable_[index.row()].characteristics, 16)).arg(PEFile::sectionCharString(static_cast<PEFile::SectionCharacteristics>(peFile_->sectionTable_[index.row()].characteristics)));
        }
        // fall through
    default:
        return QVariant();
    }
}



QVariant PESectionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section >= SECTION_FIELDS_COUNT || orientation != Qt::Horizontal)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        return sectionFields[section];
    }

    return QVariant();
}

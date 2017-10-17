#include "cofffile.h"
#include "log.h"
#include "pesection.h"
#include <QDataStream>
#include <time.h>

// These constants include the optional header extension
static const uint32_t OPTHEADER_SIZE_BASE_PE32 = 96;
static const uint32_t OPTHEADER_SIZE_BASE_PE32P = 112;

CoffFile::CoffFile() : valid_(false)
{
}

bool CoffFile::parse(QIODevice *device)
{
    QDataStream stream(device);

    stream.setByteOrder(QDataStream::LittleEndian);


    uint32_t peOffset;
    stream >> peOffset;

    device->seek(peOffset);

    if (stream.atEnd())
    {
        Log::error("Invalid PE file: stream ended");
        return false;
    }

    // PE ID
    {
        uint32_t peId;
        stream >> peId;

        if (peId != 0x4550)
        { // PE\0\0
            Log::error("Invalid PE header ID");
            return false;
        }
    }

    if (!parseCoffHeader(stream))
    {
        return false;
    }

    optionalHeaderExists_ = (coffHeader_.sizeOfOptionalHeader != 0);

    if (optionalHeaderExists_)
    {
        if (!parseOptionalHeader(stream))
        {
            return false;
        }
    }

    if (!parseSectionTable(stream))
    {
        return false;
    }

    valid_ = true;

    return true;
}

bool CoffFile::parseCoffHeader(QDataStream &stream)
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

bool CoffFile::parseOptionalHeader(QDataStream &stream)
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
            Log::error(
                "Invalid PE file: optional header is too short for PE32");
            return false;
        }
        pe32 = true;
        break;
    case ET_PE32P:
        if (coffHeader_.sizeOfOptionalHeader < OPTHEADER_SIZE_BASE_PE32P)
        {
            Log::error(
                "Invalid PE fil: optional header is too short for PE32+");
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
    // We cannot read directly to the structure because some fields are 64 bit
    // to accomodate for PE32+
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
        Log::error("Invalid PE file: data-directory entries exceed optional "
                   "header size");
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
        Log::warning(
            QString("There are %1 excess bytes in the optional header; "
                    "skipping over them")
                .arg(sizeLeft));
        stream.skipRawData(sizeLeft);
    }

    if (stream.atEnd())
    {
        Log::error("Invalid PE file: stream ended");
        return false;
    }

    return true;
}

bool CoffFile::parseSectionTable(QDataStream &stream)
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

bool CoffFile::parseSections(QDataStream &stream)
{
    for (const SectionHeader &header : sectionTable_)
    {
        PESectionPtr section = std::make_shared<PESection>(header);

        std::vector<char> data(header.sizeOfRawData);
        device_->seek(header.pointerToRawData);
        if (stream.readRawData(data.data(), header.sizeOfRawData) !=
            header.sizeOfRawData)
        {
            Log::error("Invalid PE file: stream ended reading section data");
            return false;
        }

        section->setRawData(std::move(data));

        sections_.push_back(std::static_pointer_cast<Section>(section));
    }

    return true;
}

const QString CoffFile::machineString(CoffFile::MachineType type)
{
    switch (type)
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
    CoffFile::CoffCharacteristics characteristic;
    const char *string;
};

CCharString coffCharStrings[] = {
    {CoffFile::CCHAR_RELOCS_STRIPPED, "RELOCS_STRIPPED"},
    {CoffFile::CCHAR_EXECUTABLE_IMAGE, "EXECUTABLE_IMAGE"},
    {CoffFile::CCHAR_LINE_NUMS_STRIPPED, "LINE_NUMS_STRIPPED"},
    {CoffFile::CCHAR_LOCAL_SYMS_STRIPPED, "LOCAL_SYMS_STRIPPED"},
    {CoffFile::CCHAR_AGRESSIVE_WS_TRIM, "AGRESSIVE_WS_TRIM"},
    {CoffFile::CCHAR_LARGE_ADDRESS_AWARE, "LARGE_ADDRESS_AWARE"},
    {CoffFile::CCHAR_BYTES_RESERVED_LO, "BYTES_RESERVED_LO"},
    {CoffFile::CCHAR_MACHINE_32BIT, "32BIT_MACHINE"},
    {CoffFile::CCHAR_DEBUG_STRIPPED, "DEBUG_STRIPPED"},
    {CoffFile::CCHAR_REMOVABLE_RUN_FROM_SWAP, "REMOVABLE_RUN_FROM_SWAP"},
    {CoffFile::CCHAR_NET_RUN_FROM_SWAP, "NET_RUN_FROM_SWAP"},
    {CoffFile::CCHAR_SYSTEM, "SYSTEM"},
    {CoffFile::CCHAR_DLL, "DLL"},
    {CoffFile::CCHAR_UP_SYSTEM_ONLY, "UP_SYSTEM_ONLY"},
    {CoffFile::CCHAR_BYTES_RESERVED_HI, "BYTES_RESERVED_HI"},
};

static const int COFF_CHAR_STRINGS_COUNT = 15;

const QString
CoffFile::coffCharString(CoffFile::CoffCharacteristics characteristics)
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

const QString CoffFile::subsystemString(CoffFile::Subsystem subsystem)
{
    switch (subsystem)
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
    CoffFile::DllCharacteristics characteristic;
    const char *string;
};

DCharString dllCharStrings[] = {
    {CoffFile::DCHAR_HIGHENTROPY_VA, "HIGHENTROPY_VA"},
    {CoffFile::DCHAR_DYNAMIC_BASE, "DYNAMIC_BASE"},
    {CoffFile::DCHAR_FORCE_INTEGRITY, "FORCE_INTEGRITY"},
    {CoffFile::DCHAR_NX_COMPAT, "NX_COMPAT"},
    {CoffFile::DCHAR_NO_ISOLATION, "NO_ISOLATION"},
    {CoffFile::DCHAR_NO_SEH, "NO_SEH"},
    {CoffFile::DCHAR_NO_BIND, "NO_BIND"},
    {CoffFile::DCHAR_APPCONTAINER, "APPCONTAINER"},
    {CoffFile::DCHAR_WDM_DRIVER, "WDM_DRIVER"},
    {CoffFile::DCHAR_GUARD_CF, "GUARD_CF"},
    {CoffFile::DCHAR_TERMINAL_SERVER_AWARE, "TERMINAL_SERVER_AWARE"},
};

static const int DLL_CHAR_STRINGS_COUNT = 11;

const QString
CoffFile::dllCharString(CoffFile::DllCharacteristics characteristics)
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
    CoffFile::SectionCharacteristics characteristic;
    const char *string;
};

SCharString sectionCharStrings[] = {
    {CoffFile::SCHAR_TYPE_NO_PAD, "TYPE_NO_PAD"},
    {CoffFile::SCHAR_CNT_CODE, "CNT_CODE"},
    {CoffFile::SCHAR_CNT_INITIALIZED_DATA, "CNT_INITIALIZED_DATA"},
    {CoffFile::SCHAR_CNT_UNINITIALIZED_DATA, "CNT_UNINITIALIZED_DATA"},
    {CoffFile::SCHAR_LNK_OTHER, "LNK_OTHER"},
    {CoffFile::SCHAR_LNK_INFO, "LNK_INFO"},
    {CoffFile::SCHAR_LNK_REMOVE, "LNK_REMOVE"},
    {CoffFile::SCHAR_LNK_COMDAT, "LNK_COMDAT"},
    {CoffFile::SCHAR_GPREL, "GPREL"},
    {CoffFile::SCHAR_MEM_PURGEABLE, "MEM_PURGABLE"},
    {CoffFile::SCHAR_MEM_LOCKED, "MEM_LOCKED"},
    {CoffFile::SCHAR_MEM_PRELOAD, "MEM_PRELOAD"},
    {CoffFile::SCHAR_ALGIN_1BYTES, "ALIGN_1BYTES"},
    {CoffFile::SCHAR_ALIGN_2BYTES, "ALIGN_2BYTES"},
    {CoffFile::SCHAR_ALIGN_4BYTES, "ALIGN_4BYTES"},
    {CoffFile::SCHAR_ALIGN_8BYTES, "ALIGN_8BYTES"},
    {CoffFile::SCHAR_ALIGN_16BYTES, "ALIGN_16BYTES"},
    {CoffFile::SCHAR_ALIGN_32BYTES, "ALIGN_32BYTES"},
    {CoffFile::SCHAR_ALIGN_64BYTES, "ALIGN_64BYTES"},
    {CoffFile::SCHAR_ALIGN_128BYTES, "ALIGN_128BYTES"},
    {CoffFile::SCHAR_ALIGN_256BYTES, "ALIGN_256BYTES"},
    {CoffFile::SCHAR_ALIGN_512BYTES, "ALIGN_512BYTES"},
    {CoffFile::SCHAR_ALIGN_1024BYTES, "ALIGN_1024BYTES"},
    {CoffFile::SCHAR_ALIGN_2048BYTES, "ALIGN_2048BYTES"},
    {CoffFile::SCHAR_ALIGN_4096BYTES, "ALIGN_4096BYTES"},
    {CoffFile::SCHAR_ALIGN_8192BYTES, "ALIGN_8192BYTES"},
    {CoffFile::SCHAR_LNK_NRELOC_OVFL, "LNK_NRELOC_OVFL"},
    {CoffFile::SCHAR_MEM_DISCARDABLE, "MEM_DISCARDABLE"},
    {CoffFile::SCHAR_MEM_NOT_CACHED, "MEM_NOT_CACHED"},
    {CoffFile::SCHAR_MEM_NOT_PAGED, "MEM_NOT_PAGED"},
    {CoffFile::SCHAR_MEM_SHARED, "MEM_SHARED"},
    {CoffFile::SCHAR_MEM_EXECUTE, "MEM_EXECUTE"},
    {CoffFile::SCHAR_MEM_READ, "MEM_READ"},
    {CoffFile::SCHAR_MEM_WRITE, "MEM_WRITE"},
};

static const int SECTION_CHAR_STRINGS_COUNT = 34;

const QString
CoffFile::sectionCharString(CoffFile::SectionCharacteristics characteristics)
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

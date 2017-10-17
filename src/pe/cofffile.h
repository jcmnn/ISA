#ifndef COFFFILE_H
#define COFFFILE_H

#include "section.h"
#include <QAbstractTableModel>
#include <QIODevice>
#include <memory>

class CoffFile
{
public:
    CoffFile();

    bool parse(QIODevice *device);

    inline bool valid()
    {
        return valid_;
    }

protected:
    QIODevice *device_;
    bool valid_;

private:
    bool parseCoffHeader(QDataStream &stream);
    bool parseOptionalHeader(QDataStream &stream);
    bool parseSectionTable(QDataStream &stream);
    bool parseSections(QDataStream &stream);

public:
    struct CoffHeader
    {
        uint16_t machine;
        uint16_t numberOfSections;
        uint32_t timeDateStamp;
        uint32_t pointerToSymbolTable;
        uint32_t numberOfSymbols;
        uint16_t sizeOfOptionalHeader;
        uint16_t characteristics;
    };

    struct OptionalHeader
    {
        uint16_t signature;
        uint8_t majorLinkerVersion;
        uint8_t minorLinkerVersion;
        uint32_t sizeOfCode;
        uint32_t sizeOfInitializedData;
        uint32_t sizeOfUninitialziedData;
        uint32_t addressOfEntryPoint;
        uint32_t baseOfCode;

        // PE32 only
        uint32_t baseOfData;

        // The next 21 fields are an extension to the COFF optional header
        // format
        quint64 imageBase; // 32 bit for PE32, 64 for PE32+
        uint32_t sectionAlignment;
        uint32_t fileAlignment;
        uint16_t majorOperatingSystemVersion;
        uint16_t minorOperatingSystemVersion;
        uint16_t majorImageVersion;
        uint16_t minorImageVersion;
        uint16_t majorSubsystemVersion;
        uint16_t minorSubsystemVersion;
        uint32_t win32VersionValue; // Reserved; must be zero
        uint32_t sizeOfImage;       // must be a multiple of SectionAlignment
        uint32_t sizeOfHeaders;
        uint32_t checkSum;
        uint16_t subsystem;
        uint16_t dllCharacteristics;
        quint64 sizeOfStackReserve; // 32 bit for PE32, 64 for PE32+
        quint64 sizeOfStackCommit;  // ^
        quint64 sizeOfHeapReserve;  // ^
        quint64 sizeOfHeapCommit;   // ^
        uint32_t loaderFlags;       // reserved, must be zero
        uint32_t numberOfRvaAndSizes;
    };

    struct DataDirectory
    {
        uint32_t virtualAddress;
        uint32_t size;
    };

    struct SectionHeader
    {
        char name[9]; // 8 max chars plus null-terminator
        uint32_t virtualSize;
        uint32_t virtualAddress;
        uint32_t sizeOfRawData;
        uint32_t pointerToRawData;
        uint32_t pointerToRelocations;
        uint32_t pointerToLineNumbers;
        uint16_t numberOfRelocations;
        uint16_t numberOfLineNumbers;
        uint32_t characteristics;
    };

    enum MachineType
    {
        MACH_UNKNOWN = 0x0,
        MACH_AM33 = 0x1d3,
        MACH_AMD64 = 0x8664,
        MACH_ARM = 0x1c0,
        MACH_ARM64 = 0xaa64,
        MACH_ARMNT = 0x1c4,
        MACH_EBC = 0xebc,
        MACH_I386 = 0x14c,
        MACH_IA64 = 0x200,
        MACH_M32R = 0x9041,
        MACH_MIPS16 = 0x266,
        MACH_MIPSFPU = 0x366,
        MACH_MIPSFPU16 = 0x466,
        MACH_POWERPC = 0x1f0,
        MACH_POWERPCFP = 0x1f1,
        MACH_R4000 = 0x166,
        MACH_RISCV32 = 0x5032,
        MACH_RISCV64 = 0x5064,
        MACH_RISCV128 = 0x5128,
        MACH_SH3 = 0x1a2,
        MACH_SH3DSP = 0x1a3,
        MACH_SH4 = 0x1a6,
        MACH_SH5 = 0x1a8,
        MACH_THUMB = 0x1c2,
        MACH_WCEMIPSV2 = 0x169,
    };

    enum CoffCharacteristics
    {
        CCHAR_RELOCS_STRIPPED = 0x0001,
        CCHAR_EXECUTABLE_IMAGE = 0x0002,
        CCHAR_LINE_NUMS_STRIPPED = 0x0004,  // deprecated
        CCHAR_LOCAL_SYMS_STRIPPED = 0x0008, // deprecated
        CCHAR_AGRESSIVE_WS_TRIM = 0x0010,
        CCHAR_LARGE_ADDRESS_AWARE =
            0x0020, // Application can handle > 2-GB addresses
        CCHAR_BYTES_RESERVED_LO = 0x0080, // deprecated
        CCHAR_MACHINE_32BIT = 0x0100,
        CCHAR_DEBUG_STRIPPED = 0x0200,
        CCHAR_REMOVABLE_RUN_FROM_SWAP = 0x0400,
        CCHAR_NET_RUN_FROM_SWAP = 0x0800,
        CCHAR_SYSTEM = 0x1000,
        CCHAR_DLL = 0x2000,
        CCHAR_UP_SYSTEM_ONLY =
            0x4000, // should only be run on a uniprocessor machine
        CCHAR_BYTES_RESERVED_HI = 0x8000, // deprecated
    };

    enum Subsystem
    {
        SUBSYS_UNKNOWN = 0,
        SUBSYS_NATIVE = 1,
        SUBSYS_WINDOWS_GUI = 2,
        SUBSYS_WINDOWS_CUI = 3,
        SUBSYS_OS2_CUI = 5,
        SUBSYS_POSIX_CUI = 7,
        SUBSYS_NATIVE_WINDOWS = 8,
        SUBSYS_WINDOWS_CE_GUI = 9,
        SUBSYS_EFI_APPLICATION = 10,
        SUBSYS_EFI_BOOT_SERVICE_DRIVER = 11,
        SUBSYS_EFI_RUNTIME_DRIVER = 12,
        SUBSYS_EFI_ROM = 13,
        SUBSYS_XBOX = 14,
        SUBSYS_WINDOWS_BOOT_APPLICATION = 16,
    };

    enum DllCharacteristics
    {
        DCHAR_HIGHENTROPY_VA = 0x0020,
        DCHAR_DYNAMIC_BASE = 0x0040,
        DCHAR_FORCE_INTEGRITY = 0x0080,
        DCHAR_NX_COMPAT = 0x0100,
        DCHAR_NO_ISOLATION = 0x0200,
        DCHAR_NO_SEH = 0x0400,
        DCHAR_NO_BIND = 0x0800,
        DCHAR_APPCONTAINER = 0x1000,
        DCHAR_WDM_DRIVER = 0x2000,
        DCHAR_GUARD_CF = 0x4000,
        DCHAR_TERMINAL_SERVER_AWARE = 0x8000,
    };

    enum ExeType
    {
        ET_PE32 = 0x10b,
        ET_PE32P = 0x20b,
        ET_ROM = 0x107,
    };

    enum SectionCharacteristics
    {
        SCHAR_TYPE_NO_PAD = 0x8,
        SCHAR_CNT_CODE = 0x20,
        SCHAR_CNT_INITIALIZED_DATA = 0x40,
        SCHAR_CNT_UNINITIALIZED_DATA = 0x80,
        SCHAR_LNK_OTHER = 0x100, // reserved
        SCHAR_LNK_INFO = 0x200,
        SCHAR_LNK_REMOVE = 0x800,
        SCHAR_LNK_COMDAT = 0x1000,
        SCHAR_GPREL = 0x8000,
        SCHAR_MEM_PURGEABLE = 0x20000, // reserved
        // SCHAR_MEM_16BIT = 0x20000, // reserved
        SCHAR_MEM_LOCKED = 0x40000,  // reserved
        SCHAR_MEM_PRELOAD = 0x80000, // reserved
        SCHAR_ALGIN_1BYTES = 0x100000,
        SCHAR_ALIGN_2BYTES = 0x200000,
        SCHAR_ALIGN_4BYTES = 0x300000,
        SCHAR_ALIGN_8BYTES = 0x400000,
        SCHAR_ALIGN_16BYTES = 0x500000,
        SCHAR_ALIGN_32BYTES = 0x600000,
        SCHAR_ALIGN_64BYTES = 0x700000,
        SCHAR_ALIGN_128BYTES = 0x800000,
        SCHAR_ALIGN_256BYTES = 0x900000,
        SCHAR_ALIGN_512BYTES = 0xA00000,
        SCHAR_ALIGN_1024BYTES = 0xB00000,
        SCHAR_ALIGN_2048BYTES = 0xC00000,
        SCHAR_ALIGN_4096BYTES = 0xD00000,
        SCHAR_ALIGN_8192BYTES = 0xE00000,
        SCHAR_LNK_NRELOC_OVFL = 0x1000000,
        SCHAR_MEM_DISCARDABLE = 0x2000000,
        SCHAR_MEM_NOT_CACHED = 0x4000000,
        SCHAR_MEM_NOT_PAGED = 0x8000000,
        SCHAR_MEM_SHARED = 0x10000000,
        SCHAR_MEM_EXECUTE = 0x20000000,
        SCHAR_MEM_READ = 0x40000000,
        SCHAR_MEM_WRITE = 0x80000000,
    };

    CoffHeader coffHeader_;
    bool optionalHeaderExists_;
    OptionalHeader optionalHeader_;
    std::vector<DataDirectory> dataDirectories_;
    std::vector<SectionHeader> sectionTable_;
    std::vector<SectionPtr> sections_;

    static const QString machineString(MachineType type);
    static const QString coffCharString(CoffCharacteristics characteristics);
    static const QString subsystemString(Subsystem subsystem);
    static const QString dllCharString(DllCharacteristics characteristics);
    static const QString
    sectionCharString(SectionCharacteristics characteristics);
};

typedef std::shared_ptr<CoffFile> CoffFilePtr;


#endif // COFFFILE_H

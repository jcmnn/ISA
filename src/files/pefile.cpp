#include "pefile.h"
#include <QDataStream>
#include "log.h"

// These constants include the optional header extension
static const uint32_t OPTHEADER_SIZE_BASE_PE32 = 96;
static const uint32_t OPTHEADER_SIZE_BASE_PE32P = 112;

PEFile::PEFile(QIODevice *device) : device_(device)
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

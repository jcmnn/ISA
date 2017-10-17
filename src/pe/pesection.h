#ifndef PESECTION_H
#define PESECTION_H

#include "pefile.h"
#include "section.h"
#include <vector>

class PESection : public Section
{
public:
    PESection(const PEFile::SectionHeader &header);

    bool readable() const override;
    bool executable() const override;
    bool writable() const override;

    void setRawData(std::vector<char> &&data);
    void setRawData(const std::vector<char> &data);

    uint32_t offset() override;
    uint32_t size() override;

private:
    std::vector<char> rawData_;
    PEFile::SectionHeader header_;
};

typedef std::shared_ptr<PESection> PESectionPtr;

#endif // PESECTION_H

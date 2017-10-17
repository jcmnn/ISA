#include "pesection.h"

PESection::PESection(const PEFile::SectionHeader &header) : header_(header) {}

bool PESection::readable() const {
  return (header_.characteristics & PEFile::SCHAR_MEM_READ) != 0;
}

bool PESection::executable() const {
  return (header_.characteristics & PEFile::SCHAR_MEM_EXECUTE) != 0;
}

bool PESection::writable() const {
  return (header_.characteristics & PEFile::SCHAR_MEM_WRITE) != 0;
}

void PESection::setRawData(std::vector<char> &&data) {
  rawData_ = std::move(data);
}

void PESection::setRawData(const std::vector<char> &data) { rawData_ = data; }

uint32_t PESection::offset() { return header_.virtualAddress; }

uint32_t PESection::size() { return header_.virtualSize; }

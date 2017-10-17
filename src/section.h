#ifndef SECTION_H
#define SECTION_H
#include <memory>

class Section
{
public:
    virtual ~Section(){};

    virtual bool readable() const = 0;
    virtual bool executable() const = 0;
    virtual bool writable() const = 0;

    // Returns the virtual address of the first byte in this section
    virtual uint32_t offset() = 0;

    // Returns the size of the section in bytes
    virtual uint32_t size() = 0;

private:
};

typedef std::shared_ptr<Section> SectionPtr;

#endif // SECTION_H

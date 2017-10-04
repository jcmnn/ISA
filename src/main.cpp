#include <iostream>
#include <isa.h>

int main(int argc, char *argv[])
{
    ISA isa(argc, argv);

    return isa.exec();
}

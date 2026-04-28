#include "Bitstream.h"

void Bitstream::append(const char& bin)
{
    bits += bin;
    size++;
}

void Bitstream::pop_back()
{
    bits = bits.substr(0, bits.size() - 1);
    size--;
}
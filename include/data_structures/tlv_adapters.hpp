#pragma once
#include "data_buffer.hpp"
#include "tlv.hpp"

class DataBuffer;

// DataBuffer or any others *Buffer do not need to know TLV
// register in the header

// format:
//  template <class T> *Buffer& operator<<(*Buffer& out, const T& v)
//  template <class T> *Buffer& operator>>(*Buffer& in, T& v)

template <class T> DataBuffer& operator<<(DataBuffer& out, const T& v)
{
    tlv::write_value(out, v);
    return out;
}

template <class T> DataBuffer& operator>>(DataBuffer& in, T& v)
{
    tlv::read_value(in, v);
    return in;
}

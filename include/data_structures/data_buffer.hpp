#pragma once

#include <cstddef>
#include <iostream>
#include <vector>


/*
TODO:
1. Design TLV message



0. design a TLV message to know how to send the message
1. during the compile time check the type code to decide how to serialize it
    a. if it is a POD then put it into bytes by lookup the TLV
    b. if it is a complex object then check if it implement serialize/deserialize to claim the logic
        - if it didnt impl serialize/deserialize -> raise exception
        - if it impl serialize/deserialize -> recurive serialize all the member
*/
class DataBuffer
{
public:


    template <typename U> DataBuffer& operator<<(const U& value);
    template <typename U> DataBuffer& operator>>(U& value);
    friend std::istream& operator>>(std::istream&, DataBuffer&);
    friend std::ostream& operator<<(std::ostream&, const DataBuffer&);
private:
    std::vector<std::byte> _buf;
};

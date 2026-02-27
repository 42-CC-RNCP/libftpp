#pragma once
#include <cmath>

template <typename TType> struct IVector3
{
    TType x;
    TType y;
    TType z;

    IVector3() : x(0), y(0), z(0) {}
    IVector3(TType x, TType y, TType z) : x(x), y(y), z(z) {}

    IVector3 operator+(const IVector3& other) const
    {
        return IVector3(x + other.x, y + other.y, z + other.z);
    }

    IVector3 operator-(const IVector3& other) const
    {
        return IVector3(x - other.x, y - other.y, z - other.z);
    }

    IVector3 operator*(TType scalar) const
    {
        return IVector3(x * scalar, y * scalar, z * scalar);
    }

    IVector3 operator/(TType scalar) const
    {
        return IVector3(x / scalar, y / scalar, z / scalar);
    }

    bool operator==(const IVector3& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const IVector3& other) const { return !(*this == other); }

    float length() const { return std::sqrt(x * x + y * y + z * z); }

    IVector3 normalized() const
    {
        float len = length();

        if (len == 0) {
            return IVector3(0, 0, 0);
        }
        return IVector3(x / len, y / len, z / len);
    }

    IVector3 cross(const IVector3& other) const
    {
        return IVector3(y * other.z - z * other.y,
                        z * other.x - x * other.z,
                        x * other.y - y * other.x);
    }
};

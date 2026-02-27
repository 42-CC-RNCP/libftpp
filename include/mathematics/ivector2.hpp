#pragma once
#include <cmath>

template <typename TType> struct IVector2
{
    TType x;
    TType y;

    IVector2() : x(0), y(0) {}
    IVector2(TType x, TType y) : x(x), y(y) {}

    IVector2 operator+(const IVector2& other) const
    {
        return IVector2(x + other.x, y + other.y);
    }

    IVector2 operator-(const IVector2& other) const
    {
        return IVector2(x - other.x, y - other.y);
    }

    IVector2 operator*(TType scalar) const
    {
        return IVector2(x * scalar, y * scalar);
    }

    IVector2 operator/(TType scalar) const
    {
        return IVector2(x / scalar, y / scalar);
    }

    bool operator==(const IVector2& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const IVector2& other) const { return !(*this == other); }

    float length() const { return std::sqrt(x * x + y * y); }

    IVector2 normalized() const
    {
        float len = length();

        if (len == 0) {
            return IVector2(0, 0);
        }
        return IVector2(x / len, y / len);
    }

    float dot(const IVector2& other) const { return x * other.x + y * other.y; }

    static IVector2 cross(const IVector2& a, const IVector2& b)
    {
        return IVector2(a.y * b.x - a.x * b.y, a.x * b.y - a.y * b.x);
    }
};

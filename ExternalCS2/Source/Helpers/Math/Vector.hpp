#pragma once

class Vector {
public:
    float x = 0, y = 0, z = 0;

    Vector() {}

    template<class T = float>
    Vector(T x, T y, T z) : x(x), y(y), z(z) {}

    float& operator[](int i)
    {
        return ((float*)this)[i];
    }
    float operator[](int i) const
    {
        return ((float*)this)[i];
    }

    Vector operator+(const Vector& v) const
    {
        return Vector(x + v.x, y + v.y, z + v.z);
    }

    Vector operator-(const Vector& v) const
    {
        return Vector(x - v.x, y - v.y, z - v.z);
    }
};
#pragma once

// bruh
class VMatrix {
public:
    float& operator[](int i) {
        return m[i];
    }

    float m[16];
};
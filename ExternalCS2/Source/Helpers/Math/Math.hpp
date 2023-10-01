#pragma once

#include "VMatrix.hpp"
#include "Vector.hpp"

namespace Math {
    inline int g_Width = 0;
    inline int g_Height = 0;

    struct BBox_t {
        // left up
        float x, y;
        // right down
        float w, h;
    };

    bool WorldToScreen(Vector& vecOrigin, Vector& vecScreen, VMatrix& pflViewMatrix);

    bool GetBoundingBox(Vector vecOrigin, Vector mins, Vector maxs, VMatrix pViewMatrix, BBox_t& out);
}
#include "Math.hpp"

#include <cmath>
#include <float.h>

namespace Math {
    bool WorldToScreen(Vector& vecOrigin, Vector& vecScreen, VMatrix& pflViewMatrix)
    {
        vecScreen[0] = pflViewMatrix[0] * vecOrigin[0] + pflViewMatrix[1] * vecOrigin[1] + pflViewMatrix[2] * vecOrigin[2] + pflViewMatrix[3];
        vecScreen[1] = pflViewMatrix[4] * vecOrigin[0] + pflViewMatrix[5] * vecOrigin[1] + pflViewMatrix[6] * vecOrigin[2] + pflViewMatrix[7];

        auto flTemp = pflViewMatrix[12] * vecOrigin[0] + pflViewMatrix[13] * vecOrigin[1] + pflViewMatrix[14] * vecOrigin[2] + pflViewMatrix[15];

        if (flTemp < 0.01f)
            return false;

        auto invFlTemp = 1.f / flTemp;
        vecScreen[0] *= invFlTemp;
        vecScreen[1] *= invFlTemp;

        int iResolution[2] = { 0 };
        if (!iResolution[0] || !iResolution[1])
        {
            iResolution[0] = g_Width;
            iResolution[1] = g_Height;
        }

        auto x = (float)iResolution[0] / 2.f;
        auto y = (float)iResolution[1] / 2.f;

        x += 0.5f * vecScreen[0] * (float)iResolution[0] + 0.5f;
        y -= 0.5f * vecScreen[1] * (float)iResolution[1] + 0.5f;

        vecScreen[0] = x;
        vecScreen[1] = y;

        return true;
    }

    bool GetBoundingBox(Vector vecOrigin, Vector mins, Vector maxs, VMatrix pViewMatrix, BBox_t& out)
    {
        const Vector origin = vecOrigin;
        const Vector  min = mins + origin;
        const Vector  max = maxs + origin;

        out.x = out.y = FLT_MAX;
        out.w = out.h = FLT_MIN;

        for (size_t i = 0; i < 8; ++i) {
            Vector points{
                i & 1 ? max.x : min.x, i & 2 ? max.y : min.y,
                i & 4 ? max.z : min.z
            };
            Vector screen;

            if (!WorldToScreen(points, screen, pViewMatrix))
                return false;

            out.x = std::floor(std::min< float >(out.x, screen.x));
            out.y = std::floor(std::min< float >(out.y, screen.y));
            out.w = std::floor(std::max< float >(out.w, screen.x));
            out.h = std::floor(std::max< float >(out.h, screen.y));
        }

        return true;
    }
}
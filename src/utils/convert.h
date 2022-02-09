#ifndef ERER_UTILS_CONVERT_H_
#define ERER_UTILS_CONVERT_H_

#include "../core/data_structure.hpp"
#include "../core/image.h"
#include "./tgaimage.h"

namespace Utils
{
    Core::Image<Core::Vector4c> convert_TGAImage_to_CoreImage(const Utils::TGAImage &img)
    {
        int w = img.get_width();
        int h = img.get_height();
        Core::Vector4c *data_p = new Core::Vector4c[w * h]; // delete[] is needless,and it is perfectly safe to delete nullptr
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                Utils::TGAColor pp = img.get(x, y);
                data_p[y * w + x] = Core::Vector4c{pp[2], pp[1], pp[0], pp[3]}; // bgra -> rgba
            }
        }
        return Core::Image<Core::Vector4c>(data_p, w, h);
    }
}

#endif // ERER_UTILS_CONVERT_H_
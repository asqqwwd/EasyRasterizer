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

    Utils::TGAImage convert_CoreImage_to_TGAImage(const Core::Image<float> &img)
    {
        int w = img.get_width();
        int h = img.get_height();
        Utils::TGAImage ret(w, h, 1);
        float max_dp = 0.f;
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                if (img.get(x, y) > max_dp)
                {
                    max_dp = img.get(x, y);
                }
            }
        }
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                uint8_t value = static_cast<uint8_t>(img.get(x, y) / max_dp * 255);
                ret.set(x, y, Utils::TGAColor(value, value, value));
            }
        }
        return ret;
    }

    void convert_CoreImage_to_raw_data(const Core::Image<Core::Vector3c> &img, uint8_t *out_data_p)
    {
        int w = img.get_width();
        int h = img.get_height();
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                Core::Vector3c pp = img.get(x, y);
                out_data_p[3 * (y * w + x)] = pp[0];
                out_data_p[3 * (y * w + x) + 1] = pp[1];
                out_data_p[3 * (y * w + x) + 2] = pp[2];
            }
        }
    }

    Utils::TGAImage convert_raw_data_to_TGAImage(uint8_t *in_data_p, int w, int h, int bpp)
    {
        Utils::TGAImage ret(w, h, bpp);
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                // rgba -> bgra
                ret.set(x, y, Utils::TGAColor(in_data_p[bpp * (y * w + x) + 2], in_data_p[bpp * (y * w + x) + 1], in_data_p[bpp * (y * w + x)]));
            }
        }
        return ret;
    }

}

#endif // ERER_UTILS_CONVERT_H_
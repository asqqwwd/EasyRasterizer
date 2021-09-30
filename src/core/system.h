#ifndef ERER_CORE_SYSTEM_H_
#define ERER_CORE_SYSTEM_H_

#include <iostream>
#include <vector>
#include <stdlib.h>

#include "scene.h"
#include "../settings.h"

namespace Core
{
    unsigned char *render_image = new unsigned char[Settings::WIDTH * Settings::HEIGHT * 3];

    class System;
    std::vector<System *> all_systems;

    class System
    {
    private:
    public:
        System() = delete;
        System(const System &other) = delete;
        void operator=(const System &other) = delete;
        System(System *p)
        {
            all_systems.push_back(p);
        }
        virtual void update() = 0; // pure virtual func
    };

    class RasterizeSystem : public System
    {
    private:
        // Vec3f GetBarycentricByVector(Vec2i *pts, Vec2i pt)
        // {

        //     Vec3i uv1 = Utils::CrossProduct1D(Vec3i(pts[0][0] - pts[1][0], pts[0][0] - pts[2][0], pt[0] - pts[0][0]), Vec3i(pts[0][1] - pts[1][1], pts[0][1] - pts[2][1], pt[1] - pts[0][1]));
        //     if (uv1[2] == 0)
        //     {
        //         return Vec3f(-1, -1, -1);
        //     }
        //     return Vec3f(1 - 1. * (uv1[0] + uv1[1]) / uv1[2], 1. * uv1[0] / uv1[2], 1. * uv1[1] / uv1[2]);
        // }
        // void Triangle(Tensor<int, 2> *pts, Mat &img, Tensor<unsigned char, 3> color)
        // {
        //     // Compute bbox
        //     Tensor<int, 2> bbox_min{1, 2};
        //     Tensor<int, 2> bboxMin{std::max(0, std::min({pts[0][0], pts[1][0], pts[2][0]})), std::max(0, std::min({pts[0][1], pts[1][1], pts[2][1]}))};
        //     Tensor<int, 2> bboxMax{std::min(img.cols - 1, std::max({pts[0][0], pts[1][0], pts[2][0]})), std::min(img.rows - 1, std::max({pts[0][1], pts[1][1], pts[2][1]}))};
        //     // Padding color by barycentric coordinates
        //     Tensor<int, 2> pt;
        //     for (pt[0] = bboxMin[0]; pt[0] < bboxMax[0]; pt[0]++)
        //     {
        //         for (pt[1] = bboxMin[1]; pt[1] < bboxMax[1]; pt[1]++)
        //         {
        //             // Vec3f bc = GetBarycentricByArea(pts, pt);
        //             Vec3f bc = GetBarycentricByVector(pts, pt);
        //             if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0)
        //             {
        //                 continue;
        //             }
        //             img.at<Vec3b>(pt[0], pt[1]) = color;
        //         }
        //     }
        // }

    public:
        RasterizeSystem() : System(this)
        {
        }

        void update()
        {
            for (MeshComponent *component : get_all_components<MeshComponent>())
            {
                for (auto face : component->get_all_faces())
                {
                    // Triangle()
                }
            }
        }
    };

}

#endif // ERER_CORE_SYSTEM_H_
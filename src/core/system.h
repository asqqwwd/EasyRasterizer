#ifndef ERER_CORE_SYSTEM_H_
#define ERER_CORE_SYSTEM_H_

#include <iostream>
#include <vector>
#include <stdlib.h>

#include "scene.h"
#include "shader.h"
#include "../settings.h"

namespace Core
{
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
        void triangle(Vector2i *pts, Mat &img, Tensor<unsigned char, 3> color)
        {
            // Compute bbox
            Vector2i bbox_min{1, 2};
            Vector2i bboxMin{std::max(0, std::min({pts[0][0], pts[1][0], pts[2][0]})), std::max(0, std::min({pts[0][1], pts[1][1], pts[2][1]}))};
            Vector2i bboxMax{std::min(img.cols - 1, std::max({pts[0][0], pts[1][0], pts[2][0]})), std::min(img.rows - 1, std::max({pts[0][1], pts[1][1], pts[2][1]}))};
            // Padding color by barycentric coordinates
            Vector2i pt;
            for (pt[0] = bboxMin[0]; pt[0] < bboxMax[0]; pt[0]++)
            {
                for (pt[1] = bboxMin[1]; pt[1] < bboxMax[1]; pt[1]++)
                {
                    // Vec3f bc = GetBarycentricByArea(pts, pt);
                    Vec3f bc = GetBarycentricByVector(pts, pt);
                    if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0)
                    {
                        continue;
                    }
                    img.at<Vec3b>(pt[0], pt[1]) = color;
                }
            }
        }

    public:
        RasterizeSystem() : System(this)
        {
        }

        void update()
        {
            Attribute attribute;
            Uniform uniform;
            a2v v;
            v2f i;
            for (CameraComponent *cc : get_all_components<CameraComponent>())
            {
                attribute.ViewPort = cc->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT});
                attribute.V = cc->getV();
                attribute.P = cc->getP();
                for (MeshComponent *mc : get_all_components<MeshComponent>())
                {
                    attribute.M = mc->getM();
                    for (auto face : mc->get_all_faces())
                    {
                        Tensor<v2f,3> triangle;
                        for(int i=0;i<3;++i){
                            v.POSITION = Vector3f(face[i][0][0],face[i][1][0],face[i][2][0]); 
                            v.TEXCOORD0 = Vector3f(face[i][0][1],face[i][1][1],face[i][2][1]); 
                            v.NORMAL = Vector3f(face[i][0][2],face[i][1][2],face[i][2][2]); 
                            tiangle[i] = vert(v,attribute,unifrom);
                        }
                        


                        // Triangle()
                    }
                }
            }
        }
    };

}

#endif // ERER_CORE_SYSTEM_H_
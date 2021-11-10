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
        Vector3f get_barycentric_by_vector(Vector2i *pts, Vector2i pt)
        {

            Vector3i uv = Utils::cross_product_3D(Vector3i{pts[0][0] - pts[1][0], pts[0][0] - pts[2][0], pt[0] - pts[0][0]}, Vector3i{pts[0][1] - pts[1][1], pts[0][1] - pts[2][1], pt[1] - pts[0][1]});
            if (uv[2] == 0)
            {
                return Vector3f{-1.f, -1.f, -1.f};
            }
            return Vector3f{1.f - 1.f * (uv[0] + uv[1]) / uv[2], 1.f * uv[0] / uv[2], 1.f * uv[1] / uv[2]};
        }
        void interpolation(Vector3f *pts, Vector3f *colors, Vector3f *normals)
        {
            /* Round down x and y of pts */
            Vector2i *pts_xy = new Vector2i[3];
            float *depths = new float[3];
            for (size_t i = 0; i < 3; ++i)
            {
                pts_xy[i][0] = static_cast<int>(pts[i][0]);
                pts_xy[i][1] = static_cast<int>(pts[i][1]);
                depths[i] = pts[i][2];
            }
            /* Compute bbox */
            Vector2i bbox_min{std::max(0, std::min({pts_xy[0][0], pts_xy[1][0], pts_xy[2][0]})), std::max(0, std::min({pts_xy[0][1], pts_xy[1][1], pts_xy[2][1]}))};
            Vector2i bbox_max{std::min(Settings::WIDTH - 1, std::max({pts_xy[0][0], pts_xy[1][0], pts_xy[2][0]})), std::min(Settings::HEIGHT - 1, std::max({pts_xy[0][1], pts_xy[1][1], pts_xy[2][1]}))};
            /* Padding color by barycentric coordinates */
            Tensor<float, 3> interpolated_color;
            Tensor<float, 3> interpolated_normal;
            for (int x = bbox_min[0]; x < bbox_max[0]; x++)
            {
                for (int y = bbox_min[1]; y < bbox_max[1]; y++)
                {
                    Vector3f bc = get_barycentric_by_vector(pts_xy, Vector2i{x, y});
                    if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0)
                    {
                        continue;
                    }
                    /* Depth interpolate and test */
                    float interpolated_depth = bc[0] * depths[0] + bc[1] * depths[1] + bc[2] * depths[2];
                    if (interpolated_depth <= depth_buffer_[x * Settings::WIDTH + y])
                    {
                        continue;
                    }
                    depth_buffer_[x * Settings::WIDTH + y] = interpolated_depth;
                    /* Color and Normal interpolate*/
                    for (size_t i = 0; i < 3; ++i)
                    {
                        interpolated_color[i] = bc[0] * colors[0][i] + bc[1] * colors[1][i] + bc[2] * colors[2][i];
                    }
                    for (size_t i = 0; i < 3; ++i)
                    {
                        interpolated_normal[i] = bc[0] * normals[0][i] + bc[1] * normals[1][i] + bc[2] * normals[2][i];
                    }
                    (fragments_ + x * Settings::WIDTH + y)->SV_NORMAL = interpolated_normal;
                    (fragments_ + x * Settings::WIDTH + y)->COLOR0 = interpolated_color;
                }
            }
        }

        Attribute attribute_; // system attributes
        Uniform uniform_;     // user self-defined
        VertexInput vi_;
        VertexOutput vo_;
        FragmentInput fi_;

        VertexInput *in_vertexes_;
        VertexOutput *out_vertexes_;

        FragmentInput *fragments_;
        float *depth_buffer_;          // assuming that all depth is larger than 0
        unsigned char *render_buffer_; // width*height*3, data format: rgb rgb ...

    public:
        RasterizeSystem() : System(this)
        {
            fragments_ = new FragmentInput[Settings::WIDTH * Settings::HEIGHT];
            in_vertexes_ = nullptr;
            out_vertexes_ = nullptr;

            depth_buffer_ = new float[Settings::WIDTH * Settings::HEIGHT]{};      // padding with zeros
            render_buffer_ = new unsigned char[Settings::WIDTH * Settings::HEIGHT * 3]{}; // padding with zeros
        }

        void update()
        {
            Vector3f *pts = new Vector3f[3];
            Vector3f *colors = new Vector3f[3];
            Vector3f *normals = new Vector3f[3];
            for (CameraComponent *cc : get_all_components<CameraComponent>())
            {
                attribute_.ViewPort = cc->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT});
                attribute_.V = cc->getV();
                attribute_.P = cc->getP();
                for (MeshComponent *mc : get_all_components<MeshComponent>())
                {
                    attribute_.M = mc->getM();

                    /* SOA(struct of array) to AOS(array of struct) */
                    std::vector<Vector3f> all_vertexes = mc->get_all_vertexes();
                    std::vector<Vector3f> all_uvs = mc->get_all_uvs();
                    std::vector<Vector3f> all_normals = mc->get_all_normals();
                    std::cout<<all_vertexes.size()<<" "<<all_uvs.size()<<" "<<all_normals.size()<<std::endl;
                    assert(all_vertexes.size() == all_uvs.size() == all_normals.size());
                    if (in_vertexes_ == nullptr)
                    {
                        in_vertexes_ = new VertexInput[all_vertexes.size()];
                    }
                    if (out_vertexes_ == nullptr)
                    {
                        out_vertexes_ = new VertexOutput[all_vertexes.size()];
                    }
                    for (size_t i = 0; i < all_vertexes.size(); ++i)
                    {
                        in_vertexes_[i].POSITION = all_vertexes[i];
                        in_vertexes_[i].TEXCOORD0 = all_uvs[i];
                        in_vertexes_[i].NORMAL = all_normals[i];
                    }

                    /* Pipline: vertex */
                    for (size_t i = 0; i < all_vertexes.size(); ++i)
                    {
                        out_vertexes_[i] = vert(in_vertexes_[i], attribute_, uniform_);
                    }

                    /* Pipline: barycentric cordination calculate + clip + depth test */
                    for (auto f : mc->get_all_faces())
                    {
                        for (size_t i = 0; i < 3; ++i)
                        {
                            pts[i] = out_vertexes_[f[i][0]].POSITION;
                            colors[i] = out_vertexes_[f[i][1]].COLOR0;
                            normals[i] = out_vertexes_[f[i][2]].NORMAL;
                        }
                        interpolation(pts, colors, normals);
                    }

                    // Pipline: fragment
                    for (size_t i = 0; i < Settings::WIDTH * Settings::HEIGHT; ++i)
                    {
                        Vector4i out = frag(fragments_[i]);
                        render_buffer_[i * 3] = out[0];
                        render_buffer_[i * 3 + 1] = out[1];
                        render_buffer_[i * 3 + 2] = out[2];
                    }
                    *(cc->get_render_buffer()) = *render_buffer_; // ???
                }
            }
        }
    };

}

#endif // ERER_CORE_SYSTEM_H_
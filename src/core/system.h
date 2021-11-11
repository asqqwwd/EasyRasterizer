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
        void interpolation(Vector3f *pts, Vector3i *colors, Vector3f *normals)
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
            Vector3i interpolated_color;
            Vector3f interpolated_normal;
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
                        interpolated_color[i] = static_cast<int>(bc[0] * colors[0][i] + bc[1] * colors[1][i] + bc[2] * colors[2][i]);
                        interpolated_normal[i] = bc[0] * normals[0][i] + bc[1] * normals[1][i] + bc[2] * normals[2][i];
                    }
                    fragments_[x * Settings::WIDTH + y].SV_NORMAL = interpolated_normal;
                    fragments_[x * Settings::WIDTH + y].COLOR0 = interpolated_color;
                }
            }
        }

        Attribute attribute_; // system attributes
        Uniform uniform_;     // user self-defined
        VertexInput vi_;
        VertexOutput vo_;
        FragmentInput fi_;

        std::vector<VertexInput> in_vertexes_;
        std::vector<VertexOutput> out_vertexes_;

        std::vector<FragmentInput> fragments_;
        float *depth_buffer_;          // assuming that all depth is larger than 0
        unsigned char *render_buffer_; // width*height*3, data format: rgb rgb ...

    public:
        RasterizeSystem() : System(this)
        {
            fragments_ = std::vector<FragmentInput>(Settings::WIDTH * Settings::HEIGHT);

            depth_buffer_ = new float[Settings::WIDTH * Settings::HEIGHT]{};              // padding with zeros
            render_buffer_ = new unsigned char[Settings::WIDTH * Settings::HEIGHT * 3]{}; // padding with zeros
        }

        void update()
        {
            Vector3f *pts = new Vector3f[3];
            Vector3i *colors = new Vector3i[3];
            Vector3f *normals = new Vector3f[3];
            for (CameraComponent *cc : get_all_components<CameraComponent>())
            {
                attribute_.ViewPort = cc->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT});
                attribute_.V = cc->getV();
                attribute_.P = cc->getP();
                for (MeshComponent *mc : get_all_components<MeshComponent>())
                {
                    attribute_.M = mc->getM();

                    /* Load point data */
                    auto all_faces = mc->get_all_faces();
                    in_vertexes_.clear();
                    for (size_t i = 0; i < all_faces.size(); ++i)
                    {
                        for (size_t j = 0; j < 3; ++j)
                        {
                            vi_.POSITION = all_faces[i][j][0];
                            vi_.TEXCOORD0 = all_faces[i][j][1];
                            vi_.NORMAL = all_faces[i][j][2];
                            in_vertexes_.push_back(vi_);
                        }
                    }

                    /* Pipline: vertex */
                    out_vertexes_.clear();
                    for (auto vi : in_vertexes_)
                    {
                        vo_ = vert(vi, attribute_, uniform_);
                        out_vertexes_.push_back(vo_);
                    }

                    /* Pipline: barycentric cordination calculate + clip + depth test */
                    for (size_t i = 0; i < out_vertexes_.size(); i = i + 3)
                    {
                        for (size_t j = 0; j < 3; ++j)
                        {
                            pts[j] = out_vertexes_[i + j].POSITION;
                            colors[j] = out_vertexes_[i + j].COLOR0;
                            normals[j] = out_vertexes_[i + j].NORMAL;
                        }
                        interpolation(pts, colors, normals);
                    }

                    // Pipline: fragment
                    for (size_t i = 0; i < fragments_.size(); ++i)
                    {
                        Vector4i out = frag(fragments_[i]);
                        render_buffer_[i * 3] = out[0];
                        render_buffer_[i * 3 + 1] = out[1];
                        render_buffer_[i * 3 + 2] = out[2];
                    }
                    std::copy(render_buffer_, render_buffer_ + Settings::WIDTH * Settings::HEIGHT * 3, cc->get_render_buffer());
                }
            }
        }

        unsigned char *get_render_buffer()
        {
            return render_buffer_;
        }
    };

}

#endif // ERER_CORE_SYSTEM_H_
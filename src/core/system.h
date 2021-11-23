#ifndef ERER_CORE_SYSTEM_H_
#define ERER_CORE_SYSTEM_H_

#include <iostream>
#include <vector>
#include <memory>   // for smart pointer
#include <stdlib.h> // for math
#include <float.h>  // for FLT_MAX

#include "scene.h"
#include "shader.h"
#include "time.h"
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
        Attribute attribute_; // system attributes
        Uniform uniform_;     // user self-defined

        std::vector<VertexOutput> out_vertexes_;

        std::vector<FragmentInput> fragments_;
        float *depth_buffer_;          // assuming that all depth is larger than 0
        unsigned char *render_buffer_; // width*height*3, data format: rgb rgb ...

        Vector3f get_barycentric_by_vector(Vector2i *pts, Vector2i pt)
        {
            Vector3i uv = Utils::cross_product_3D(Vector3i{pts[0][0] - pts[1][0], pts[0][0] - pts[2][0], pt[0] - pts[0][0]}, Vector3i{pts[0][1] - pts[1][1], pts[0][1] - pts[2][1], pt[1] - pts[0][1]});
            if (uv[2] == 0)
            {
                return Vector3f{-1.f, -1.f, -1.f};
            }
            return Vector3f{1.f - 1.f * (uv[0] + uv[1]) / uv[2], 1.f * uv[0] / uv[2], 1.f * uv[1] / uv[2]};
        }
        void interpolation(Vector3f *pts, Vector3f *colors, Vector3f *normals, Vector3f *uvs)
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
            Vector3f interpolated_color;
            Vector3f interpolated_normal;
            Vector3f interpolated_uv;
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
                    if (interpolated_depth >= depth_buffer_[y * Settings::WIDTH + x])
                    {
                        continue;
                    }
                    depth_buffer_[y * Settings::WIDTH + x] = interpolated_depth;
                    /* Color and Normal interpolate*/
                    for (size_t i = 0; i < 3; ++i)
                    {
                        interpolated_color[i] = bc[0] * colors[0][i] + bc[1] * colors[1][i] + bc[2] * colors[2][i];
                        interpolated_normal[i] = bc[0] * normals[0][i] + bc[1] * normals[1][i] + bc[2] * normals[2][i];
                        interpolated_uv[i] = bc[0] * uvs[0][i] + bc[1] * uvs[1][i] + bc[2] * uvs[2][i];
                    }
                    fragments_[y * Settings::WIDTH + x].IWS_NORMAL = interpolated_normal;
                    fragments_[y * Settings::WIDTH + x].I_UV = interpolated_uv;
                    fragments_[y * Settings::WIDTH + x].I_COLOR = interpolated_color;
                }
            }
        }

    public:
        RasterizeSystem() : System(this)
        {
            depth_buffer_ = new float[Settings::WIDTH * Settings::HEIGHT]{};              // padding with zeros
            render_buffer_ = new unsigned char[Settings::WIDTH * Settings::HEIGHT * 3]{}; // padding with zeros
        }

        void update()
        {
            auto pts = std::make_unique<Vector3f[]>(3);
            auto colors = std::make_unique<Vector3f[]>(3);
            auto normals = std::make_unique<Vector3f[]>(3);
            auto uvs = std::make_unique<Vector3f[]>(3);
            for (CameraComponent *cc : get_all_components<CameraComponent>())
            {
                for (LightComponent *lc : get_all_components<LightComponent>())
                {
                    attribute_.V = cc->getV();
                    attribute_.P = cc->getP();
                    attribute_.ViewPort = cc->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT});

                    attribute_.world_light_dir = lc->get_light_dir();
                    attribute_.light_color = lc->get_light_color();
                    attribute_.light_intensity = lc->get_light_intensity();

                    attribute_.world_view_dir = cc->get_lookat_dir();
                    attribute_.specular_color = lc->get_specular_color();

                    attribute_.ambient = lc->get_ambient_color();

                    for (MeshComponent *mc : get_all_components<MeshComponent>())
                    {

                        attribute_.M = mc->getM();
                        uniform_.albedo = mc->get_albedo_texture();
                        uniform_.gloss = mc->get_gloss();

                        /* Pipline: vertex */
                        out_vertexes_.clear(); // just move insert pointer to the start without deleting origin space
                        for (auto vi : mc->get_all_vertexes())
                        {
                            // out_vertexes_.push_back(GouraudShader::vert(vi, attribute_, uniform_));
                            out_vertexes_.push_back(PhongShader::vert(vi, attribute_, uniform_));
                        }

                        /* Pipline: barycentric cordination calculate + clip + depth test */
                        fragments_.clear();
                        fragments_.shrink_to_fit();
                        fragments_ = std::vector<FragmentInput>(Settings::WIDTH * Settings::HEIGHT);
                        std::fill(depth_buffer_, depth_buffer_ + Settings::WIDTH * Settings::HEIGHT, FLT_MAX);
                        for (size_t i = 0; i < out_vertexes_.size(); i = i + 3)
                        {
                            for (size_t j = 0; j < 3; ++j)
                            {
                                pts[j] = out_vertexes_[i + j].SS_POSITION;
                                normals[j] = out_vertexes_[i + j].WS_NORMAL;
                                uvs[j] = out_vertexes_[i + j].UV;
                                colors[j] = out_vertexes_[i + j].COLOR;
                            }
                            interpolation(pts.get(), colors.get(), normals.get(), uvs.get());
                        }

                        // Pipline: fragment
                        for (size_t i = 0; i < fragments_.size(); ++i)
                        {
                            // Vector4i out = GouraudShader::frag(fragments_[i], attribute_, uniform_);
                            Vector4i out = PhongShader::frag(fragments_[i], attribute_, uniform_);
                            render_buffer_[i * 3] = out[0];
                            render_buffer_[i * 3 + 1] = out[1];
                            render_buffer_[i * 3 + 2] = out[2];
                        }
                        std::copy(render_buffer_, render_buffer_ + Settings::WIDTH * Settings::HEIGHT * 3, cc->get_render_buffer());
                    }
                }
            }
        }
    };

    class MotionSystem : public System
    {
    private:
    public:
        MotionSystem() : System(this)
        {
        }

        void update()
        {
            // Test
            // std::cout << Time::get_delta_time() << std::endl;
            double w = 0.5;
            static double theta = 0;
            theta += w * Time::get_delta_time();
            get_entity("HeadObj")->get_component<MeshComponent>()->set_rotation(Vector3f{static_cast<float>(std::cos(theta)), 0, static_cast<float>(std::sin(theta))}, Vector3f{0, 1, 0});
        }
    };

}

#endif // ERER_CORE_SYSTEM_H_
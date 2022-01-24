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
        // std::vector<VertexOutput> out_vertexes_;
        // std::vector<FragmentInput> fragments_;
        float *depth_buffer_;          // assuming that all depth is larger than 0
        unsigned char *render_buffer_; // width*height*3, data format: bgr bgr ...

        Vector3f get_barycentric_by_vector(const Vector2i pts[3], const Vector2i &pt)
        {
            Vector3i uv = Utils::cross_product_3D(Vector3i{pts[0][0] - pts[1][0], pts[0][0] - pts[2][0], pt[0] - pts[0][0]}, Vector3i{pts[0][1] - pts[1][1], pts[0][1] - pts[2][1], pt[1] - pts[0][1]});
            if (std::fabs(uv[2]) <= 1e-10)
            {
                return Vector3f{-1.f, -1.f, -1.f};
            }
            return Vector3f{1.f - 1.f * (uv[0] + uv[1]) / uv[2], 1.f * uv[0] / uv[2], 1.f * uv[1] / uv[2]};
        }

    public:
        RasterizeSystem() : System(this)
        {
            depth_buffer_ = new float[Settings::WIDTH * Settings::HEIGHT]{};              // padding with zeros
            render_buffer_ = new unsigned char[Settings::WIDTH * Settings::HEIGHT * 3]{}; // padding with zeros
        }

        void update()
        {
            Attribute attribute; // system attributes
            Uniform uniform;     // user self-defined
            auto vo_of_3pts = std::make_unique<VertexOutput[]>(3);
            FragmentInput fragment_input;
            Vector4i fragment_out;
            Matrix4f M_view_port;
            /* Pass 1 */
            CameraComponent *main_light_camera = new CameraComponent(0.1f, 20.f, 90.f, 90.f);
            main_light_camera->set_position(current_scene->get_entity("MainLight")->get_component<LightComponent>()->get_position());
            main_light_camera->lookat_with_fixed_up(current_scene->get_entity("MainLight")->get_component<LightComponent>()->get_light_dir());
            attribute.V = main_light_camera->getV();
            attribute.P = main_light_camera->getP();
            M_view_port = main_light_camera->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT});
            float *shadow_map = new float[Settings::WIDTH * Settings::HEIGHT]{};
            std::fill(shadow_map, shadow_map + Settings::WIDTH * Settings::HEIGHT, main_light_camera->get_far());
            for (MeshComponent *mc : get_all_components<MeshComponent>())
            {
                attribute.M = mc->getM();
                std::vector<VertexInput> in_vertexes = mc->get_all_vertexes();
                for (size_t i = 0; i < in_vertexes.size(); i += 3)
                {
                    /* Pipline: vertex */
                    for (size_t j = 0; j < 3; ++j)
                    {
                        vo_of_3pts[j] = PhongShader::vert(in_vertexes[i + j], attribute, uniform);
                    }

                    /* Pipline: barycentric cordination calculate + clip + depth test */
                    // Clip space->Screen space
                    auto SS_pos_of_3pts = std::make_unique<Vector4f[]>(3); // screen space postion of 3 points
                    for (size_t i = 0; i < 3; ++i)
                    {
                        SS_pos_of_3pts[i] = M_view_port.mul(vo_of_3pts[i].CS_POSITION / vo_of_3pts[i].CS_POSITION[3]);
                    }
                    // Round down x and y of pts
                    auto pt3_xy = std::make_unique<Vector2i[]>(3);
                    for (size_t k = 0; k < 3; ++k)
                    {
                        pt3_xy[k][0] = static_cast<int>(SS_pos_of_3pts.get()[k][0]);
                        pt3_xy[k][1] = static_cast<int>(SS_pos_of_3pts.get()[k][1]);
                    }
                    //Compute bbox
                    Vector2i bbox_min{std::max(0, std::min({pt3_xy[0][0], pt3_xy[1][0], pt3_xy[2][0]})), std::max(0, std::min({pt3_xy[0][1], pt3_xy[1][1], pt3_xy[2][1]}))};
                    Vector2i bbox_max{std::min(Settings::WIDTH - 1, std::max({pt3_xy[0][0], pt3_xy[1][0], pt3_xy[2][0]})), std::min(Settings::HEIGHT - 1, std::max({pt3_xy[0][1], pt3_xy[1][1], pt3_xy[2][1]}))};
                    //Padding color by barycentric coordinates
                    for (int x = bbox_min[0]; x < bbox_max[0]; x++)
                    {
                        for (int y = bbox_min[1]; y < bbox_max[1]; y++)
                        {
                            Vector3f bc_screen = get_barycentric_by_vector(pt3_xy.get(), Vector2i{x, y});
                            if (bc_screen[0] < 0 || bc_screen[1] < 0 || bc_screen[2] < 0)
                            {
                                continue;
                            }
                            // Depth interpolate and test
                            Vector3f bc_clip = {bc_screen[0] / vo_of_3pts[0].CS_POSITION[3], bc_screen[1] / vo_of_3pts[1].CS_POSITION[3], bc_screen[2] / vo_of_3pts[2].CS_POSITION[3]};
                            float Z_n = 1 / (bc_clip[0] + bc_clip[1] + bc_clip[2]);
                            if (Z_n >= shadow_map[y * Settings::WIDTH + x] || Z_n < main_light_camera->get_near() || Z_n > main_light_camera->get_far())
                            {
                                continue;
                            }
                            shadow_map[y * Settings::WIDTH + x] = Z_n;
                        }
                    }
                }
            }
            uint8_t *save_data = new uint8_t[Settings::WIDTH * Settings::HEIGHT];
            for (int i = 0; i < Settings::WIDTH * Settings::HEIGHT; ++i)
            {
                save_data[i] = static_cast<uint8_t>(Utils::saturate((shadow_map[i] - main_light_camera->get_near()) / (main_light_camera->get_far() - main_light_camera->get_near())) * 255);
            }
            Utils::save_tga_image("../../shadowmap.tga", Settings::WIDTH, Settings::HEIGHT, 1, save_data);
            /* Pass 2 */
            for (CameraComponent *cc : get_all_components<CameraComponent>())
            {
                attribute.V = cc->getV();
                attribute.P = cc->getP();
                attribute.camera_postion = cc->get_position();
                M_view_port = cc->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT});
                for (LightComponent *lc : get_all_components<LightComponent>())
                {
                    attribute.world_light_dir = lc->get_light_dir();
                    attribute.light_color = lc->get_light_color();
                    attribute.light_intensity = lc->get_light_intensity();

                    attribute.specular_color = lc->get_specular_color();

                    attribute.ambient = lc->get_ambient_color();

                    std::fill(depth_buffer_, depth_buffer_ + Settings::WIDTH * Settings::HEIGHT, cc->get_far());
                    for (MeshComponent *mc : get_all_components<MeshComponent>())
                    {
                        attribute.M = mc->getM();
                        uniform.albedo = mc->get_albedo_texture();
                        uniform.gloss = mc->get_gloss();

                        std::vector<VertexInput> in_vertexes = mc->get_all_vertexes();
                        for (size_t i = 0; i < in_vertexes.size(); i += 3)
                        {
                            /* Pipline: vertex */
                            for (size_t j = 0; j < 3; ++j)
                            {
                                vo_of_3pts[j] = PhongShader::vert(in_vertexes[i + j], attribute, uniform);
                            }

                            /* Pipline: barycentric cordination calculate + clip + depth test */
                            // Clip space->Screen space
                            auto SS_pos_of_3pts = std::make_unique<Vector4f[]>(3); // screen space postion of 3 points
                            for (size_t i = 0; i < 3; ++i)
                            {
                                SS_pos_of_3pts[i] = M_view_port.mul(vo_of_3pts[i].CS_POSITION / vo_of_3pts[i].CS_POSITION[3]);
                            }
                            // Round down x and y of pts
                            auto pt3_xy = std::make_unique<Vector2i[]>(3);
                            for (size_t k = 0; k < 3; ++k)
                            {
                                pt3_xy[k][0] = static_cast<int>(SS_pos_of_3pts.get()[k][0]);
                                pt3_xy[k][1] = static_cast<int>(SS_pos_of_3pts.get()[k][1]);
                            }
                            //Compute bbox
                            Vector2i bbox_min{std::max(0, std::min({pt3_xy[0][0], pt3_xy[1][0], pt3_xy[2][0]})), std::max(0, std::min({pt3_xy[0][1], pt3_xy[1][1], pt3_xy[2][1]}))};
                            Vector2i bbox_max{std::min(Settings::WIDTH - 1, std::max({pt3_xy[0][0], pt3_xy[1][0], pt3_xy[2][0]})), std::min(Settings::HEIGHT - 1, std::max({pt3_xy[0][1], pt3_xy[1][1], pt3_xy[2][1]}))};
                            //Padding color by barycentric coordinates
                            for (int x = bbox_min[0]; x < bbox_max[0]; x++)
                            {
                                for (int y = bbox_min[1]; y < bbox_max[1]; y++)
                                {
                                    Vector3f bc_screen = get_barycentric_by_vector(pt3_xy.get(), Vector2i{x, y});
                                    if (bc_screen[0] < 0 || bc_screen[1] < 0 || bc_screen[2] < 0)
                                    {
                                        continue;
                                    }
                                    // Depth interpolate and test
                                    Vector3f bc_clip = {bc_screen[0] / vo_of_3pts[0].CS_POSITION[3], bc_screen[1] / vo_of_3pts[1].CS_POSITION[3], bc_screen[2] / vo_of_3pts[2].CS_POSITION[3]};
                                    float Z_n = 1 / (bc_clip[0] + bc_clip[1] + bc_clip[2]);
                                    if (Z_n >= depth_buffer_[y * Settings::WIDTH + x] || Z_n < cc->get_near() || Z_n > cc->get_far())
                                    {
                                        continue;
                                    }
                                    depth_buffer_[y * Settings::WIDTH + x] = Z_n;
                                    //Color and Normal interpolate
                                    fragment_input.I_UV = (bc_clip[0] * vo_of_3pts[0].UV + bc_clip[1] * vo_of_3pts[1].UV + bc_clip[2] * vo_of_3pts[2].UV) * Z_n;
                                    fragment_input.IWS_NORMAL = (bc_clip[0] * vo_of_3pts[0].WS_NORMAL + bc_clip[1] * vo_of_3pts[1].WS_NORMAL + bc_clip[2] * vo_of_3pts[2].WS_NORMAL) * Z_n;
                                    fragment_input.I_COLOR = (bc_clip[0] * vo_of_3pts[0].COLOR + bc_clip[1] * vo_of_3pts[1].COLOR + bc_clip[2] * vo_of_3pts[2].COLOR) * Z_n;
                                    fragment_input.IWS_POSITION = (bc_clip[0] * vo_of_3pts[0].WS_POSITION + bc_clip[1] * vo_of_3pts[1].WS_POSITION + bc_clip[2] * vo_of_3pts[2].WS_POSITION) * Z_n;
                                    /* Pipline: fragment */
                                    fragment_out = PhongShader::frag(fragment_input, attribute, uniform);
                                    // rgb -> bgr
                                    render_buffer_[(y * Settings::WIDTH + x) * 3] = fragment_out[2];
                                    render_buffer_[(y * Settings::WIDTH + x) * 3 + 1] = fragment_out[1];
                                    render_buffer_[(y * Settings::WIDTH + x) * 3 + 2] = fragment_out[0];
                                }
                            }
                        }
                    }
                }

                std::copy(render_buffer_, render_buffer_ + Settings::WIDTH * Settings::HEIGHT * 3, cc->get_render_buffer());
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
            std::cout << Time::get_delta_time() << std::endl;
            double w = 0.5;
            static double theta = 0;
            theta += w * Time::get_delta_time();
            get_entity("HeadObj")->get_component<MeshComponent>()->set_rotation(Vector3f{static_cast<float>(std::cos(theta)), 0, static_cast<float>(std::sin(theta))}, Vector3f{0, 1, 0});
        }
    };
}

#endif // ERER_CORE_SYSTEM_H_
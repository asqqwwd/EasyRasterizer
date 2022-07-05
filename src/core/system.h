#ifndef ERER_CORE_SYSTEM_H_
#define ERER_CORE_SYSTEM_H_

#include <iostream>
#include <vector>
#include <memory>   // for smart pointer
#include <stdlib.h> // for math
#include <float.h>  // for FLT_MAX
#include <chrono>   // for std::chrono
#include <random>   // for std::default_random_engine

#include "scene.h"
#include "shader.h"
#include "time.h"
#include "../settings.h"
#include "../utils/math.h"

namespace Core
{
    class System;
    std::vector<System*> all_systems;

    class System
    {
    private:
    public:
        System() = delete;
        System(const System& other) = delete;
        void operator=(const System& other) = delete;
        System(System* p)
        {
            all_systems.push_back(p);
        }
        virtual void update() = 0; // pure virtual func
    };

    class RasterizeSystem : public System
    {
    private:
        std::vector<CameraComponent*> depth_cameras_;

        template <typename T>
        using Triangle = Tensor<T, 3>;

        template <typename T, int Dim>
        class Plane
        {
        private:
            Tensor<Tensor<T, Dim>, 2> data_;

        public:
            Plane()
            {
            }
            Plane(const Tensor<T, Dim>& point, const Tensor<T, Dim>& normal)
            {
                data_[0] = point;
                data_[1] = normal.normal(); // normal is for gettting length directly after projection
            }
            Tensor<T, Dim>& P()
            {
                return data_[0];
            }
            Tensor<T, Dim> P() const
            {
                return data_[0];
            }
            Tensor<T, Dim>& N()
            {
                return data_[1];
            }
            Tensor<T, Dim> N() const
            {
                return data_[1];
            }
        };

        Plane<float, 4> homogeneous_space_planes_[7] = {
            Plane(Vector4f{0, 0, 0, 0.00001f}, Vector4f{0, 0, 0, -1}), // w
            Plane(Vector4f{0, 0, 0, 0}, Vector4f{1, 0, 0, -1}),        // left
            Plane(Vector4f{0, 0, 0, 0}, Vector4f{-1, 0, 0, -1}),       // right
            Plane(Vector4f{0, 0, 0, 0}, Vector4f{0, 1, 0, -1}),        // top
            Plane(Vector4f{0, 0, 0, 0}, Vector4f{0, -1, 0, -1}),       // down
            Plane(Vector4f{0, 0, 0, 0}, Vector4f{0, 0, 1, -1}),        // far
            Plane(Vector4f{0, 0, 0, 0}, Vector4f{0, 0, -1, -1}),       // near
        };

        Vector3f get_barycentric_by_vector(const Triangle<Vector2f>& triangle, const Vector2f& pt)
        {
            Vector3f uv = Utils::cross_product_3D(Vector3f{ triangle[0][0] - triangle[1][0], triangle[0][0] - triangle[2][0], pt[0] - triangle[0][0] }, Vector3f{ triangle[0][1] - triangle[1][1], triangle[0][1] - triangle[2][1], pt[1] - triangle[0][1] });
            if (std::fabs(uv[2]) <= 1e-10)
            {
                return Vector3f{ -1.f, -1.f, -1.f };
            }
            return Vector3f{ 1.f - (uv[0] + uv[1]) / uv[2], uv[0] / uv[2], uv[1] / uv[2] };
        }

        // Return N triangle(s), N~{0,1,2}
        std::vector<Triangle<VertexOutput>> triangle_clipping(const Triangle<VertexOutput>& triangle, const Plane<float, 4>& plane)
        {
            std::vector<Triangle<VertexOutput>> ret;
            std::vector<float> ds;
            std::vector<VertexOutput> saved_pts;
            // clipping points
            for (auto pt : triangle)
            {
                ds.push_back(Utils::dot_product(pt.CS_POSITION - plane.P(), plane.N()));
            }
            for (int i = 0; i < 3; ++i)
            {
                if (ds[i] <= 0)
                {
                    saved_pts.push_back(triangle[i]);
                }
                if (ds[i] * ds[(i + 1) % 3] < 0)
                {
                    saved_pts.push_back(Utils::lerp(triangle[i], triangle[(i + 1) % 3], std::fabs(ds[i]) / (std::fabs(ds[i]) + std::fabs(ds[(i + 1) % 3]))));
                }
            }
            // re-assembling triangles using new and old points
            if (saved_pts.size() == 3)
            {
                ret.push_back(Triangle<VertexOutput>{saved_pts[0], saved_pts[1], saved_pts[2]});
            }
            else if (saved_pts.size() == 4)
            {
                ret.push_back(Triangle<VertexOutput>{saved_pts[0], saved_pts[1], saved_pts[2]});
                ret.push_back(Triangle<VertexOutput>{saved_pts[2], saved_pts[3], saved_pts[0]});
            }
            return ret;
        }

        std::vector<Triangle<VertexOutput>> homogeneous_clipping(const std::vector<Triangle<VertexOutput>>& triangles, int plane_type)
        {
            if (plane_type == 7 || triangles.size() == 0)
            {
                return triangles;
            }
            std::vector<Triangle<VertexOutput>> new_triangles;
            for (auto tri : triangles)
            {
                std::vector<Triangle<VertexOutput>> res = triangle_clipping(tri, homogeneous_space_planes_[plane_type]);
                new_triangles.insert(new_triangles.end(), res.begin(), res.end());
            }
            return homogeneous_clipping(new_triangles, ++plane_type);
        }

        float HS(const Image<float>& shadow_map, const Vector4f& WS_pos, const Vector3f& WS_normal, const Vector3f& camera_look_at_dir, const Matrix4f& V, const Matrix4f& P, const Matrix4f& ViewPort, float eps = 0.01f)
        {
            Vector4f CS_pos = P.mul(V.mul(WS_pos));
            float view_space_depth = CS_pos[3];
            CS_pos = CS_pos / CS_pos[3];
            float u = (CS_pos[0] + 1) / 2;
            float v = (CS_pos[1] + 1) / 2;
            float correct_eps = eps / std::fabs(Utils::dot_product(WS_normal.normal(), camera_look_at_dir.normal()));
            float first_depth = shadow_map.sampling(u, v);
            return (view_space_depth - first_depth > correct_eps) ? 0.f : 1.f;
        }

        float PCF(const Image<float>& shadow_map, const Vector4f& WS_pos, const Vector3f& WS_normal, const Vector3f& camera_look_at_dir, const Matrix4f& V, const Matrix4f& P, const Matrix4f& ViewPort, float eps = 0.05f, float filter_size = 0.02f)
        {
            Vector4f CS_pos = P.mul(V.mul(WS_pos));
            float d_receiver = CS_pos[3];

            CS_pos = CS_pos / CS_pos[3];
            float u = (CS_pos[0] + 1) / 2;
            float v = (CS_pos[1] + 1) / 2;
            float correct_eps = eps / std::pow(std::fabs(Utils::dot_product(WS_normal.normal(), camera_look_at_dir.normal())), 4.f);

            // Random hit
            std::vector<Vector2f> rd_samples = Utils::RandomSample::poissonDiskSamples(10, 50);
            bool flag = false;
            for (const auto& rd_sample : rd_samples)
            {
                float d_blocker = shadow_map.sampling(u + rd_sample[0] * filter_size, v + rd_sample[1] * filter_size);
                if (d_receiver - d_blocker > correct_eps)
                {
                    flag = true;
                    break;
                }
            }
            if (!flag) // hit miss, return directly
            {
                return 1.f;
            }

            float avg_viz = 0.f;
            rd_samples = Utils::RandomSample::poissonDiskSamples(10, 100);
            // std::vector<Vector2f> rd_samples = Utils::RandomSample::uniformDiskSamples();
            // float correct_eps = std::max(eps * (1.0f - std::fabs(Utils::dot_product(WS_normal.normal(), camera_look_at_dir.normal()))), 0.03f);
            for (const auto& rd_sample : rd_samples)
            {
                float d_blocker = shadow_map.sampling(u + rd_sample[0] * filter_size, v + rd_sample[1] * filter_size);
                avg_viz += (d_receiver - d_blocker > correct_eps) ? 0.f : 1.f;
            }
            avg_viz /= rd_samples.size();
            return avg_viz;
        }

        float PCSS(const Image<float>& shadow_map, const Vector4f& WS_pos, const Vector3f& WS_normal, const Vector3f& camera_look_at_dir, const Matrix4f& V, const Matrix4f& P, const Matrix4f& ViewPort, float eps = 0.05f, float w_light = 1.f)
        {
            Vector4f CS_pos = P.mul(V.mul(WS_pos));
            float d_receiver = CS_pos[3];

            CS_pos = CS_pos / CS_pos[3];
            float u = (CS_pos[0] + 1) / 2;
            float v = (CS_pos[1] + 1) / 2;
            float correct_eps = eps / std::pow(std::fabs(Utils::dot_product(WS_normal.normal(), camera_look_at_dir.normal())), 4.f);
            float filter_size = 0.15f * w_light / d_receiver; // the farther the distance, the smaller the filter_size

            // Blocker search
            std::vector<Vector2f> rd_samples = Utils::RandomSample::poissonDiskSamples(5, 150);
            float d_blocker_avg = 0.f;
            int k = 0;
            for (const auto& rd_sample : rd_samples)
            {
                float d_blocker = shadow_map.sampling(u + rd_sample[0] * filter_size, v + rd_sample[1] * filter_size);
                if (d_receiver - d_blocker > correct_eps)
                {
                    d_blocker_avg += d_blocker;
                    ++k;
                }
            }
            if (k == 0) // hit miss, return directly
            {
                return 1.f;
            }
            d_blocker_avg /= k;

            // Dynamic filter_size
            float avg_viz = 0.f;
            rd_samples = Utils::RandomSample::poissonDiskSamples(5, 150);
            float dynamic_filter_size = 0.001f * w_light * k * (d_receiver - d_blocker_avg) / d_blocker_avg;
            // float dynamic_filter_size = filter_size * (d_receiver - d_blocker_avg) / d_blocker_avg;
            for (const auto& rd_sample : rd_samples)
            {
                float d_blocker_rd = shadow_map.sampling(u + rd_sample[0] * dynamic_filter_size, v + rd_sample[1] * dynamic_filter_size);
                avg_viz += (d_receiver - d_blocker_rd > eps) ? 0.f : 1.f;
            }
            avg_viz /= rd_samples.size();
            return avg_viz;
        }

        void Pass(const std::vector<MeshComponent*>& meshes, const std::vector<LightComponent*>& lights, const std::vector<CameraComponent*>& cameras, bool ZWrite = true, bool ZTest = true, bool ColorWrite = true)
        {
            if (meshes.size() == 0 || lights.size() == 0 || cameras.size() == 0)
            {
                return;
            }
            for (CameraComponent* camera : cameras)
            {
                // Buffer flush
                camera->flush_buffer();

                // Getting attributes
                CameraAttribute ca; // object attributes
                ca.V = camera->getV();
                ca.P = camera->getP();
                ca.camera_postion = camera->get_position();

                for (LightComponent* light : lights)
                {
                    // Getting attributes
                    LightAttribute la; // light attributes
                    la.world_light_dir = light->get_light_dir();
                    la.light_color = light->get_light_color();
                    la.light_intensity = light->get_light_intensity();
                    la.specular_color = light->get_specular_color();
                    la.ambient = light->get_ambient_color();

                    for (MeshComponent* mesh : meshes)
                    {
                        // Getting attributes
                        MeshAttribute ma; // mesh attribute
                        ma.M = mesh->getM();
                        ma.albedo = mesh->get_albedo_texture();
                        ma.gloss = mesh->get_gloss();

                        const std::vector<VertexInput>& in_vertexes = mesh->get_all_vertexes();
                        for (size_t i = 0; i < in_vertexes.size(); i += 3)
                        {
                            /* Pipline: vertex */
                            Triangle<VertexOutput> vo3;
                            for (int j = 0; j < 3; ++j)
                            {
                                vo3[j] = PhongShader::vert(in_vertexes[i + j], ca, ma);
                            }

                            // Homogeneous clipping
                            std::vector<Triangle<VertexOutput>> clip_tris = homogeneous_clipping(std::vector<Triangle<VertexOutput>>{vo3}, 0);

                            for (auto tri : clip_tris)
                            {
                                // Perspective division
                                Triangle<Vector4f> SS_pos3; // screen space postion of 3 points
                                Matrix4f M_view_port = camera->getViewPort(Vector2i{ Settings::WIDTH, Settings::HEIGHT });
                                for (int i = 0; i < 3; ++i)
                                {
                                    SS_pos3[i] = M_view_port.mul(tri[i].CS_POSITION / tri[i].CS_POSITION[3]);
                                }
                                // Round down x and y of pts
                                Triangle<Vector2f> xy3;
                                for (int k = 0; k < 3; ++k) {
                                    xy3[k][0] = SS_pos3[k][0];
                                    xy3[k][1] = SS_pos3[k][1];
                                }
                                // for (int k = 0; k < 3; ++k)
                                // {
                                //     xy3[k][0] = static_cast<int>(SS_pos3[k][0]);
                                //     xy3[k][1] = static_cast<int>(SS_pos3[k][1]);
                                // }
                                // Compute bbox
                                Vector2f bbox_min{ std::max(0.f, std::min({xy3[0][0], xy3[1][0], xy3[2][0]})), std::max(0.f, std::min({xy3[0][1], xy3[1][1], xy3[2][1]})) };
                                Vector2f bbox_max{ std::min(Settings::WIDTH - 1.f, std::max({xy3[0][0], xy3[1][0], xy3[2][0]})), std::min(Settings::HEIGHT - 1.f, std::max({xy3[0][1], xy3[1][1], xy3[2][1]})) };
                                Vector2i bbox_min_i{ static_cast<int>(std::round(bbox_min[0])),static_cast<int>(std::round(bbox_min[1])) };
                                Vector2i bbox_max_i{ static_cast<int>(std::round(bbox_max[0])),static_cast<int>(std::round(bbox_max[1])) };
                                // Padding color by barycentric coordinates
                                for (int x = bbox_min_i[0]; x < bbox_max_i[0]; x++) {
                                    for (int y = bbox_min_i[1]; y < bbox_max_i[1]; y++) {
                                        Vector3f bc_screen;
                                        float cover_rate = 0.f;
                                        float tmp[2]{ -0.25f,0.25f };
                                        // float tmp[2]{ 0.f,0.f };
                                        for (int k = 0;k < 4;++k) {  // 4X MSAA
                                            bc_screen = get_barycentric_by_vector(xy3, Vector2f{ x + tmp[k % 2],y + tmp[k / 2] });
                                            if (bc_screen[0] < 0 || bc_screen[1] < 0 || bc_screen[2] < 0) {
                                                continue;
                                            }
                                            cover_rate += 0.25f;
                                        }
                                        if (cover_rate < 1e-10) {
                                            continue;
                                        }
                                        bc_screen = get_barycentric_by_vector(xy3, Vector2f{ x + 0.f,y + 0.f });
                                        // Depth interpolate and test
                                        Vector3f bc_clip = { bc_screen[0] / tri[0].CS_POSITION[3], bc_screen[1] / tri[1].CS_POSITION[3], bc_screen[2] / tri[2].CS_POSITION[3] };
                                        float Z_n = 1 / (bc_clip[0] + bc_clip[1] + bc_clip[2]);
                                        if (ZTest) {
                                            if (Z_n >= camera->get_depth_buffer().get(x, y)) {
                                                continue;
                                            }
                                        }
                                        if (ZWrite) {
                                            camera->set_depth_buffer(x, y, Z_n);
                                        }
                                        if (camera->type != CameraComponent::Type::ColorCamera) {
                                            continue;
                                        }
                                        // Color and Normal interpolate
                                        VertexOutput interp_vo;
                                        for (int i = 0; i < 3; ++i) {
                                            interp_vo += bc_clip[i] * tri[i];
                                        }
                                        FragmentInput fi;
                                        fi.I_UV = interp_vo.UV * Z_n;
                                        fi.IWS_NORMAL = interp_vo.WS_NORMAL * Z_n;
                                        fi.IWS_POSITION = interp_vo.WS_POSITION * Z_n;

                                        /* Pipline: fragment */
                                        Vector4c fo = PhongShader::frag(fi, la, ca, ma, cover_rate);
                                        /* Visibility test for creating shadow */
                                        float visibility = 0.f;
                                        for (auto dp_camera : depth_cameras_) {
                                            // visibility += HS(dp_camera->get_depth_buffer(), fi.IWS_POSITION, fi.IWS_NORMAL, dp_camera->get_lookat_dir(), dp_camera->getV(), dp_camera->getP(), dp_camera->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT}));
                                            // visibility += PCF(dp_camera->get_depth_buffer(), fi.IWS_POSITION, fi.IWS_NORMAL, dp_camera->get_lookat_dir(), dp_camera->getV(), dp_camera->getP(), dp_camera->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT}));
                                            visibility += PCSS(dp_camera->get_depth_buffer(), fi.IWS_POSITION, fi.IWS_NORMAL, dp_camera->get_lookat_dir(), dp_camera->getV(), dp_camera->getP(), dp_camera->getViewPort(Vector2i{ Settings::WIDTH, Settings::HEIGHT }));
                                        }
                                        // fo *= visibility;
                                        // fo *= cover_rate;
                                        if (ColorWrite) {
                                            camera->set_color_buffer(x, y, fo);
                                        }
                                    }
                                }
                            } // end for trianle
                        }     // end for vertex
                    }         // end for mesh
                }             // end for light
            }                 // end for camera
        }

    public:
        RasterizeSystem() : System(this)
        {
        }

        void update()
        {
            // std::vector<Vector2f> rd_samples = Utils::RandomSample::poissonDiskSamples();
            // for (auto rd_sample : rd_samples)
            // {
            //     std::cout << rd_sample;
            // }
            // std::cout << "**" << std::endl;
            // rd_samples = Utils::RandomSample::poissonDiskSamples();
            // for (auto rd_sample : rd_samples)
            // {
            //     std::cout << rd_sample;
            // }
            // return;

            auto cameras = current_scene->get_all_components<CameraComponent>();
            decltype(cameras) color_cameras;
            decltype(cameras) depth_cameras;
            for (auto camera : cameras)
            {
                switch (camera->type)
                {
                case CameraComponent::Type::ColorCamera:
                    color_cameras.push_back(camera);
                    break;
                case CameraComponent::Type::DepthCamera:
                    depth_cameras.push_back(camera);
                    break;
                default:
                    throw std::exception("Unknown camera type!\n");
                    break;
                }
            }
            assert(color_cameras.size() == 1);

            auto meshes = current_scene->get_all_components<MeshComponent>();
            decltype(meshes) opaque_meshes;
            decltype(meshes) transparent_meshes;
            for (auto mesh : meshes)
            {
                switch (mesh->type)
                {
                case MeshComponent::Type::Opaque:
                    opaque_meshes.push_back(mesh);
                    break;
                case MeshComponent::Type::Transparent:
                    transparent_meshes.push_back(mesh);
                    break;
                default:
                    throw std::exception("Unknown mesh type!\n");
                    break;
                }
            }

            // Sort by view space distance
            auto increase_cmp_func = [](const MeshComponent* comp1, const MeshComponent* comp2) -> bool
            {
                return (*comp1).Z_view < (*comp2).Z_view;
            };
            auto decrease_cmp_func = [](const MeshComponent* comp1, const MeshComponent* comp2) -> bool
            {
                return (*comp1).Z_view > (*comp2).Z_view;
            };
            for (auto color_camera : color_cameras)
            {
                for (auto mesh : meshes)
                {
                    mesh->Z_view = color_camera->getV().mul(mesh->get_position().reshape<4>(1))[2];
                }
                std::sort(opaque_meshes.begin(), opaque_meshes.end(), increase_cmp_func);
                std::sort(transparent_meshes.begin(), transparent_meshes.end(), decrease_cmp_func);
            }

            // Depth camera render
            auto lights = current_scene->get_all_components<LightComponent>();
            Pass(meshes, lights, depth_cameras);
            depth_cameras_ = depth_cameras;

            // Color camera render
            Pass(opaque_meshes, lights, color_cameras);
            Pass(transparent_meshes, lights, color_cameras);
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
            get_entity("TestObj")->get_component<MeshComponent>()->set_rotation(Vector3f{ static_cast<float>(std::cos(theta)), 0, static_cast<float>(std::sin(theta)) }, Vector3f{ 0, 1, 0 });
        }
    };
}

#endif // ERER_CORE_SYSTEM_H_
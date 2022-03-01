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
        std::vector<CameraComponent *> depth_cameras_;

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
            Plane(const Tensor<T, Dim> &point, const Tensor<T, Dim> &normal)
            {
                data_[0] = point;
                data_[1] = normal.normal(); // normal is for gettting length directly after projection
            }
            Tensor<T, Dim> &P()
            {
                return data_[0];
            }
            Tensor<T, Dim> P() const
            {
                return data_[0];
            }
            Tensor<T, Dim> &N()
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

        Vector3f get_barycentric_by_vector(const Triangle<Vector2i> &triangle, const Vector2i &pt)
        {
            Vector3i uv = Utils::cross_product_3D(Vector3i{triangle[0][0] - triangle[1][0], triangle[0][0] - triangle[2][0], pt[0] - triangle[0][0]}, Vector3i{triangle[0][1] - triangle[1][1], triangle[0][1] - triangle[2][1], pt[1] - triangle[0][1]});
            if (std::fabs(uv[2]) <= 1e-10)
            {
                return Vector3f{-1.f, -1.f, -1.f};
            }
            return Vector3f{1.f - 1.f * (uv[0] + uv[1]) / uv[2], 1.f * uv[0] / uv[2], 1.f * uv[1] / uv[2]};
        }

        // Return N triangle(s), N~{0,1,2}
        std::vector<Triangle<VertexOutput>> triangle_clipping(const Triangle<VertexOutput> &triangle, const Plane<float, 4> &plane)
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

        std::vector<Triangle<VertexOutput>> homogeneous_clipping(const std::vector<Triangle<VertexOutput>> &triangles, int plane_type)
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

        float HS(const Image<float> &shadow_map, const Vector4f &WS_pos, const Matrix4f &V, const Matrix4f &P, const Matrix4f &ViewPort)
        {
            Vector4f NDC_pos = P.mul(V.mul(WS_pos));
            float view_space_depth = NDC_pos[3];
            NDC_pos = NDC_pos / NDC_pos[3];
            float u = (NDC_pos[0] + 1) / 2;
            float v = (NDC_pos[1] + 1) / 2;
            float first_depth = shadow_map.sampling(u, v);
            return (view_space_depth - first_depth > 0.05f) ? 0.f : 1.f;
        }
        void Pass(const std::vector<MeshComponent *> &meshes, const std::vector<LightComponent *> &lights, const std::vector<CameraComponent *> &cameras, bool ZWrite = true, bool ZTest = true, bool ColorWrite = true)
        {
            for (CameraComponent *camera : cameras)
            {
                // Buffer flush
                camera->flush_buffer();

                // Getting attributes
                CameraAttribute ca; // object attributes
                ca.V = camera->getV();
                ca.P = camera->getP();
                ca.camera_postion = camera->get_position();

                for (LightComponent *light : lights)
                {
                    // Getting attributes
                    LightAttribute la; // light attributes
                    la.world_light_dir = light->get_light_dir();
                    la.light_color = light->get_light_color();
                    la.light_intensity = light->get_light_intensity();
                    la.specular_color = light->get_specular_color();
                    la.ambient = light->get_ambient_color();

                    for (MeshComponent *mesh : meshes)
                    {
                        // Getting attributes
                        MeshAttribute ma; // mesh attribute
                        ma.M = mesh->getM();
                        ma.albedo = mesh->get_albedo_texture();
                        ma.gloss = mesh->get_gloss();

                        const std::vector<VertexInput> &in_vertexes = mesh->get_all_vertexes();
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
                                Matrix4f M_view_port = camera->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT});
                                for (int i = 0; i < 3; ++i)
                                {
                                    SS_pos3[i] = M_view_port.mul(tri[i].CS_POSITION / tri[i].CS_POSITION[3]);
                                }
                                // Round down x and y of pts
                                Triangle<Vector2i> xy3;
                                for (int k = 0; k < 3; ++k)
                                {
                                    xy3[k][0] = static_cast<int>(SS_pos3[k][0]);
                                    xy3[k][1] = static_cast<int>(SS_pos3[k][1]);
                                }
                                // Compute bbox
                                Vector2i bbox_min{std::max(0, std::min({xy3[0][0], xy3[1][0], xy3[2][0]})), std::max(0, std::min({xy3[0][1], xy3[1][1], xy3[2][1]}))};
                                Vector2i bbox_max{std::min(Settings::WIDTH - 1, std::max({xy3[0][0], xy3[1][0], xy3[2][0]})), std::min(Settings::HEIGHT - 1, std::max({xy3[0][1], xy3[1][1], xy3[2][1]}))};
                                // Padding color by barycentric coordinates
                                for (int x = bbox_min[0]; x < bbox_max[0]; x++)
                                {
                                    for (int y = bbox_min[1]; y < bbox_max[1]; y++)
                                    {
                                        Vector3f bc_screen = get_barycentric_by_vector(xy3, Vector2i{x, y});
                                        if (bc_screen[0] < 0 || bc_screen[1] < 0 || bc_screen[2] < 0)
                                        {
                                            continue;
                                        }
                                        // Depth interpolate and test
                                        Vector3f bc_clip = {bc_screen[0] / tri[0].CS_POSITION[3], bc_screen[1] / tri[1].CS_POSITION[3], bc_screen[2] / tri[2].CS_POSITION[3]};
                                        float Z_n = 1 / (bc_clip[0] + bc_clip[1] + bc_clip[2]);
                                        if (ZTest)
                                        {
                                            if (Z_n >= camera->get_depth_buffer().get(x, y))
                                            {
                                                continue;
                                            }
                                        }
                                        if (ZWrite)
                                        {
                                            camera->set_depth_buffer(x, y, Z_n);
                                        }
                                        if (camera->type != CameraComponent::Type::ColorCamera)
                                        {
                                            continue;
                                        }
                                        // Color and Normal interpolate
                                        VertexOutput interp_vo;
                                        for (int i = 0; i < 3; ++i)
                                        {
                                            interp_vo += bc_clip[i] * tri[i];
                                        }
                                        FragmentInput fi;
                                        fi.I_UV = interp_vo.UV * Z_n;
                                        fi.IWS_NORMAL = interp_vo.WS_NORMAL * Z_n;
                                        fi.IWS_POSITION = interp_vo.WS_POSITION * Z_n;
                                        /* Pipline: fragment */
                                        Vector4c fo = PhongShader::frag(fi, la, ca, ma);
                                        /* Visibility test for creating shadow */
                                        float visibility = 0.f;
                                        for (auto dp_camera : depth_cameras_)
                                        {
                                            visibility += HS(dp_camera->get_depth_buffer(), fi.IWS_POSITION.reshape<4>(1), dp_camera->getV(), dp_camera->getP(), dp_camera->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT}));
                                        }
                                        fo *= visibility;
                                        if (ColorWrite)
                                        {
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

        class SortByDist
        {
        private:
            static std::default_random_engine e_;
            static Matrix4f V_;
            static float wrapper_func(const MeshComponent &comp, const Matrix4f &V)
            {
                return V.mul(comp.get_position().reshape<4>(1))[2];
            }
            static bool com_func(const MeshComponent &comp1, const MeshComponent &comp2, const Matrix4f &V)
            {
                return wrapper_func(comp1, V) < wrapper_func(comp2, V) ? true : false;
            }
            static int partition(std::vector<MeshComponent *> &elms, int l, int r)
            {
                MeshComponent *pivot = elms[r];
                int i = l - 1;
                for (int j = l; j <= r - 1; ++j)
                {
                    if (com_func(*(elms[j]), *pivot, V_))
                    {
                        i = i + 1;
                        std::swap(elms[i], elms[j]);
                    }
                }
                std::swap(elms[i + 1], elms[r]);
                return i + 1;
            }
            static int randomized_partition(std::vector<MeshComponent *> &elms, int l, int r)
            {
                int i = e_() % (r - l + 1) + l; // choose a random element as pivot
                std::swap(elms[r], elms[i]);
                return partition(elms, l, r);
            }
            static void randomized_quicksort(std::vector<MeshComponent *> &elms, int l, int r)
            {
                if (l < r)
                {
                    int pos = randomized_partition(elms, l, r);
                    randomized_quicksort(elms, l, pos - 1);
                    randomized_quicksort(elms, pos + 1, r);
                }
            }

        public:
            static void run(std::vector<MeshComponent *> &elms, const Matrix4f &V)
            {
                V_ = V;
                e_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
                randomized_quicksort(elms, 0, static_cast<int>(elms.size()) - 1);
            }
        };

    public:
        RasterizeSystem() : System(this)
        {
        }

        void update()
        {
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
            decltype(meshes) transparent_cameras;
            for (auto mesh : meshes)
            {
                switch (mesh->type)
                {
                case MeshComponent::Type::Opaque:
                    opaque_meshes.push_back(mesh);
                    break;
                case MeshComponent::Type::Transparent:
                    transparent_cameras.push_back(mesh);
                    break;
                default:
                    throw std::exception("Unknown mesh type!\n");
                    break;
                }
            }
            // // Sort by view space distance
            // for (auto color_camera : color_cameras)
            // {
            //     SortByDist::run(meshes, color_camera->getV());
            // }

            // Depth camera render
            auto lights = current_scene->get_all_components<LightComponent>();
            Pass(meshes, lights, depth_cameras);
            depth_cameras_ = depth_cameras;

            // Color camera render
            Pass(meshes, lights, color_cameras);
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
            get_entity("TestObj")->get_component<MeshComponent>()->set_rotation(Vector3f{static_cast<float>(std::cos(theta)), 0, static_cast<float>(std::sin(theta))}, Vector3f{0, 1, 0});
        }
    };
}

#endif // ERER_CORE_SYSTEM_H_
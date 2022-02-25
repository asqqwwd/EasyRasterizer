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
        class Pass
        {
        private:
            std::vector<MeshComponent *> objs_;
            std::vector<LightComponent *> lights_;
            std::vector<CameraComponent *> cameras_;

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

        public:
            bool ZWrite = true;
            bool ZTest = true;
            bool ColorWrite = true;
            Pass(const std::vector<MeshComponent *> &objs, const std::vector<LightComponent *> &lights, const std::vector<CameraComponent *> &cameras) : objs_(objs), lights_(lights), cameras_(cameras)
            {
            }
            void run()
            {
                Attribute attribute; // system attributes
                Uniform uniform;     // user self-defined
                for (CameraComponent *camera : cameras_)
                {
                    // Getting attributes
                    attribute.V = camera->getV();
                    attribute.P = camera->getP();
                    attribute.camera_postion = camera->get_position();

                    // Buffer flush
                    camera->flush_buffer();

                    for (LightComponent *light : lights_)
                    {
                        // Getting attributes
                        attribute.world_light_dir = light->get_light_dir();
                        attribute.light_color = light->get_light_color();
                        attribute.light_intensity = light->get_light_intensity();
                        attribute.specular_color = light->get_specular_color();
                        attribute.ambient = light->get_ambient_color();

                        for (MeshComponent *obj : objs_)
                        {
                            // Getting attributes
                            attribute.M = obj->getM();
                            uniform.albedo = obj->get_albedo_texture();
                            uniform.gloss = obj->get_gloss();

                            std::vector<VertexInput> in_vertexes = obj->get_all_vertexes();
                            for (size_t i = 0; i < in_vertexes.size(); i += 3)
                            {
                                /* Pipline: vertex */
                                Triangle<VertexOutput> vo_of_3pts;
                                for (int j = 0; j < 3; ++j)
                                {
                                    vo_of_3pts[j] = PhongShader::vert(in_vertexes[i + j], attribute, uniform);
                                }
                                // Homogeneous clipping
                                std::vector<Triangle<VertexOutput>> clipped_triangles = homogeneous_clipping(std::vector<Triangle<VertexOutput>>{vo_of_3pts}, 0);
                                // std::vector<Triangle<VertexOutput>> clipped_triangles{vo_of_3pts};
                                for (auto tri : clipped_triangles)
                                {
                                    // std::cout << "clip space _" << std::endl;
                                    // for (auto pt : tri)
                                    // {
                                    //     std::cout << pt.UV;
                                    // }
                                    // Perspective division
                                    Triangle<Vector4f> SS_pos_of_3pts; // screen space postion of 3 points
                                    Matrix4f M_view_port = camera->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT});
                                    // std::cout << "screen space _" << std::endl;
                                    for (int i = 0; i < 3; ++i)
                                    {
                                        SS_pos_of_3pts[i] = M_view_port.mul(tri[i].CS_POSITION / tri[i].CS_POSITION[3]);
                                        // std::cout << SS_pos_of_3pts[i];
                                    }
                                    // Round down x and y of pts
                                    Triangle<Vector2i> xy_of_3pts;
                                    for (int k = 0; k < 3; ++k)
                                    {
                                        xy_of_3pts[k][0] = static_cast<int>(SS_pos_of_3pts[k][0]);
                                        xy_of_3pts[k][1] = static_cast<int>(SS_pos_of_3pts[k][1]);
                                    }
                                    //Compute bbox
                                    Vector2i bbox_min{std::max(0, std::min({xy_of_3pts[0][0], xy_of_3pts[1][0], xy_of_3pts[2][0]})), std::max(0, std::min({xy_of_3pts[0][1], xy_of_3pts[1][1], xy_of_3pts[2][1]}))};
                                    Vector2i bbox_max{std::min(Settings::WIDTH - 1, std::max({xy_of_3pts[0][0], xy_of_3pts[1][0], xy_of_3pts[2][0]})), std::min(Settings::HEIGHT - 1, std::max({xy_of_3pts[0][1], xy_of_3pts[1][1], xy_of_3pts[2][1]}))};
                                    //Padding color by barycentric coordinates
                                    for (int x = bbox_min[0]; x < bbox_max[0]; x++)
                                    {
                                        for (int y = bbox_min[1]; y < bbox_max[1]; y++)
                                        {
                                            Vector3f bc_screen = get_barycentric_by_vector(xy_of_3pts, Vector2i{x, y});
                                            if (bc_screen[0] < 0 || bc_screen[1] < 0 || bc_screen[2] < 0)
                                            {
                                                continue;
                                            }
                                            // Depth interpolate and test
                                            Vector3f bc_clip = {bc_screen[0] / tri[0].CS_POSITION[3], bc_screen[1] / tri[1].CS_POSITION[3], bc_screen[2] / tri[2].CS_POSITION[3]};
                                            float Z_n = 1 / (bc_clip[0] + bc_clip[1] + bc_clip[2]);
                                            if (Z_n >= depth_buffer.get(x, y))
                                            {
                                                continue;
                                            }
                                            depth_buffer.set(x, y, Z_n); // view space depth with correction
                                            //Color and Normal interpolate
                                            VertexOutput itpr_vo;
                                            for (int i = 0; i < 3; ++i)
                                            {
                                                itpr_vo += bc_clip[i] * tri[i];
                                            }
                                            FragmentInput fragment_input;
                                            fragment_input.I_UV = itpr_vo.UV * Z_n;
                                            fragment_input.IWS_NORMAL = itpr_vo.WS_NORMAL * Z_n;
                                            fragment_input.IWS_POSITION = itpr_vo.WS_POSITION * Z_n;
                                            /* Pipline: fragment */
                                            Vector4c fragment_out = PhongShader::frag(fragment_input, attribute, uniform);
                                            /* Visibility test for creating shadow */
                                            // float visibility = HS(fragment_input.IWS_POSITION.reshape<4>(1), main_light_camera->getV(), main_light_camera->getP(), main_light_camera->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT}));
                                            // fragment_out *= visibility;
                                            // rgb -> bgr
                                            render_buffer[(y * Settings::WIDTH + x) * 3] = fragment_out[2];
                                            render_buffer[(y * Settings::WIDTH + x) * 3 + 1] = fragment_out[1];
                                            render_buffer[(y * Settings::WIDTH + x) * 3 + 2] = fragment_out[0];
                                        }
                                    }
                                } // end for trianle
                            }     // end for vertex
                        }         // end for obj
                    }             // end for light
                }                 // end for camera
            }
        };

    public:
        RasterizeSystem() : System(this)
        {
        }

        void update()
        {

            /* Pass 1 */
            // CameraComponent *main_light_camera = new CameraComponent(0.1f, 20.f, 90.f, 90.f);
            // main_light_camera->set_position(current_scene->get_entity("MainLight")->get_component<LightComponent>()->get_position());
            // main_light_camera->lookat_with_fixed_up(current_scene->get_entity("MainLight")->get_component<LightComponent>()->get_light_dir());
            // attribute.V = main_light_camera->getV();
            // attribute.P = main_light_camera->getP();
            // M_view_port = main_light_camera->getViewPort(Vector2i{Settings::WIDTH, Settings::HEIGHT});
            // shadow_map_.memset(main_light_camera->get_far());
            // for (MeshComponent *mc : get_all_components<MeshComponent>())
            // {
            //     attribute.M = mc->getM();
            //     std::vector<VertexInput> in_vertexes = mc->get_all_vertexes();
            //     for (size_t i = 0; i < in_vertexes.size(); i += 3)
            //     {
            //         /* Pipline: vertex */
            //         for (size_t j = 0; j < 3; ++j)
            //         {
            //             vo_of_3pts[j] = PhongShader::vert(in_vertexes[i + j], attribute, uniform);
            //         }

            //         /* Pipline: barycentric cordination calculate + clip + depth test */
            //         // Clip space->Screen space
            //         auto SS_pos_of_3pts = std::make_unique<Vector4f[]>(3); // screen space postion of 3 points
            //         for (size_t i = 0; i < 3; ++i)
            //         {
            //             SS_pos_of_3pts[i] = M_view_port.mul(vo_of_3pts[i].CS_POSITION / vo_of_3pts[i].CS_POSITION[3]);
            //         }
            //         // Round down x and y of pts
            //         auto pt3_xy = std::make_unique<Vector2i[]>(3);
            //         for (size_t k = 0; k < 3; ++k)
            //         {
            //             pt3_xy[k][0] = static_cast<int>(SS_pos_of_3pts.get()[k][0]);
            //             pt3_xy[k][1] = static_cast<int>(SS_pos_of_3pts.get()[k][1]);
            //         }
            //         //Compute bbox
            //         Vector2i bbox_min{std::max(0, std::min({pt3_xy[0][0], pt3_xy[1][0], pt3_xy[2][0]})), std::max(0, std::min({pt3_xy[0][1], pt3_xy[1][1], pt3_xy[2][1]}))};
            //         Vector2i bbox_max{std::min(Settings::WIDTH - 1, std::max({pt3_xy[0][0], pt3_xy[1][0], pt3_xy[2][0]})), std::min(Settings::HEIGHT - 1, std::max({pt3_xy[0][1], pt3_xy[1][1], pt3_xy[2][1]}))};
            //         //Padding color by barycentric coordinates
            //         for (int x = bbox_min[0]; x < bbox_max[0]; x++)
            //         {
            //             for (int y = bbox_min[1]; y < bbox_max[1]; y++)
            //             {
            //                 Vector3f bc_screen = get_barycentric_by_vector(pt3_xy.get(), Vector2i{x, y});
            //                 if (bc_screen[0] < 0 || bc_screen[1] < 0 || bc_screen[2] < 0)
            //                 {
            //                     continue;
            //                 }
            //                 // Depth interpolate and test
            //                 Vector3f bc_clip = {bc_screen[0] / vo_of_3pts[0].CS_POSITION[3], bc_screen[1] / vo_of_3pts[1].CS_POSITION[3], bc_screen[2] / vo_of_3pts[2].CS_POSITION[3]};
            //                 float Z_n = 1 / (bc_clip[0] + bc_clip[1] + bc_clip[2]);
            //                 if (Z_n >= shadow_map_.get(x, y) || Z_n < main_light_camera->get_near() || Z_n > main_light_camera->get_far())
            //                 {
            //                     continue;
            //                 }
            //                 shadow_map_.set(x, y, Z_n); // view space depth with correction
            //             }
            //         }
            //     }
            // }

            /* Pass 2 */
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
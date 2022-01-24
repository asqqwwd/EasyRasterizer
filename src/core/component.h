#ifndef ERER_CORE_COMPONENT_H_
#define ERER_CORE_COMPONENT_H_

#include <iostream>
#include <vector>
#include <string>
#include <typeinfo>
#include <cmath>

#include "data_structure.hpp"
#include "shader.h"
#include "tgaimage.h"
#include "../utils/loader.h"
#include "../utils/math.h"
#include "../settings.h"

namespace Core
{
    class Component
    {
    protected:
        Matrix4f T_model_;
        Matrix4f S_model_;
        Matrix4f R_model_;

    public:
        Component()
        {
            T_model_ = indentity<float, 4>();
            S_model_ = indentity<float, 4>();
            R_model_ = indentity<float, 4>();
        }
        virtual ~Component() {} // virtual func is necessary, otherwise dynamic_cast(base->drived) will fail
        Component *set_position(const Vector3f &pos)
        {
            T_model_[0][3] = pos[0];
            T_model_[1][3] = pos[1];
            T_model_[2][3] = pos[2];
            return this;
        }
        Component *set_scala(const Vector3f &scala)
        {
            S_model_[0][0] = scala[0];
            S_model_[1][1] = scala[1];
            S_model_[2][2] = scala[2];
            return this;
        }
        Component *set_rotation(const Vector3f &x, const Vector3f &y)
        {
            Vector3f w = x.normal();
            Vector3f u = y.normal();
            Vector3f v = Utils::cross_product_3D(w, u).normal();
            for (size_t i = 0; i < 3; ++i)
            {
                R_model_[i][0] = w[i]; // x->x
                R_model_[i][1] = u[i]; // y->y
                R_model_[i][2] = v[i]; // z->cross(x,y)
            }
            return this;
        }
        Vector3f get_position()
        {
            return Vector3f{T_model_[0][3], T_model_[1][3], T_model_[2][3]};
        }
        Vector3f get_scala()
        {
            return Vector3f{S_model_[0][0], S_model_[1][1], S_model_[2][2]};
        }

        Matrix4f getM()
        {
            return T_model_.mul(R_model_.mul(S_model_));
        }
    };

    class MeshComponent : public Component
    {
    private:
        std::vector<VertexInput> in_vertexes_;
        TGAImage albedo_;
        float gloass_;

    public:
        MeshComponent() : gloass_(10)
        {
        }

        MeshComponent *load_vertexes(const std::string &filename)
        {
            std::vector<Vector3f> positions;
            std::vector<Vector3f> uvs;
            std::vector<Vector3f> normals;
            std::vector<Matrix3i> faces;
            Utils::load_obj_file(filename, &positions, &uvs, &normals, &faces);

            VertexInput tmp;
            for (auto f : faces)
            {
                for (size_t i = 0; i < 3; ++i)
                {
                    tmp.MS_POSITION = positions[f[0][i]];
                    tmp.UV = uvs[f[1][i]];
                    tmp.MS_NORMAL = normals[f[2][i]];
                    in_vertexes_.push_back(tmp);
                }
            }
            assert(in_vertexes_.size() == faces.size() * 3);
            return this;
        }

        MeshComponent *load_albedo_texture(const std::string &filename)
        {
            albedo_.read_tga_file(filename);
            return this;
        }

        const std::vector<VertexInput> &get_all_vertexes()
        {
            return in_vertexes_;
        }

        const TGAImage &get_albedo_texture()
        {
            return albedo_;
        }

        float get_gloss()
        {
            return gloass_;
        }
    };

    class CameraComponent : public Component
    {
    private:
        unsigned char *render_buffer_;

        float near_;
        float far_;
        float vertical_angle_of_view_;
        float horizontal_angle_of_view_;

        Matrix4f R_view_;
        Matrix4f T_view_;
        Matrix4f M_persp2ortho_;
        Matrix4f M_ortho_;

    public:
        CameraComponent(float near_sp = 0.1f, float far_sp = 10.f, float vertical_angle_of_view = 90.f, float horizontal_angle_of_view = 90.f)
            : render_buffer_(new unsigned char[Settings::WIDTH * Settings::HEIGHT * 3]), near_(near_sp), far_(far_sp), vertical_angle_of_view_(vertical_angle_of_view), horizontal_angle_of_view_(horizontal_angle_of_view)
        {
            // near/far is keyword in the windows system! near_sp/far_sp is a substitute for near/far
            M_persp2ortho_ = Matrix4f{{near_, 0, 0, 0}, {0, near_, 0, 0}, {0, 0, near_ + far_, -near_ * far_}, {0, 0, 1, 0}};
            M_ortho_ = Matrix4f{{static_cast<float>(1 / (near_ * std::tan(horizontal_angle_of_view_ / 360 * PI))), 0, 0, 0}, {0, static_cast<float>(1 / (near_ * std::tan(vertical_angle_of_view_ / 360 * PI))), 0, 0}, {0, 0, 2 / (far_ - near_), -(near_ + far_) / (far_ - near_)}, {0, 0, 0, 1}};
        }

        CameraComponent *lookat(const Vector3f &gaze, const Vector3f &up)
        {
            Vector3f w = gaze.normal();
            Vector3f u = up.normal();
            Vector3f v = Utils::cross_product_3D(w, u).normal();
            for (int i = 0; i < 3; ++i)
            {
                R_model_[i][0] = v[i]; // x->cross(gaze,up)
                R_model_[i][1] = u[i]; // y->up
                R_model_[i][2] = w[i]; // z->gaze
            }
            R_view_ = R_model_.transpose();
            T_view_ = T_model_;
            T_view_[0][3] *= -1;
            T_view_[1][3] *= -1;
            T_view_[2][3] *= -1;
            return this;
        }

        CameraComponent *lookat_with_fixed_up(const Vector3f &gaze)
        {
            Vector3f w = gaze.normal();
            Vector3f v = Vector3f{-w[2], 0, w[0]}.normal();
            Vector3f u = Utils::cross_product_3D(v, w).normal();
            for (int i = 0; i < 3; ++i)
            {
                R_model_[i][0] = v[i]; // x->cross(gaze,up)
                R_model_[i][1] = u[i]; // y->up
                R_model_[i][2] = w[i]; // z->gaze
            }
            R_view_ = R_model_.transpose();
            return this;
        }

        // redifine instead of overload or override
        CameraComponent *set_position(const Vector3f &pos)
        {
            T_model_[0][3] = pos[0];
            T_model_[1][3] = pos[1];
            T_model_[2][3] = pos[2];
            T_view_ = T_model_;
            T_view_[0][3] *= -1;
            T_view_[1][3] *= -1;
            T_view_[2][3] *= -1;
            return this;
        }

        unsigned char *get_render_buffer()
        {
            return render_buffer_;
        }

        Matrix4f getV()
        {
            return R_view_.mul(T_view_);
        }

        Matrix4f getP()
        {
            return M_ortho_.mul(M_persp2ortho_);
        }

        Matrix4f getVP()
        {
            return M_ortho_.mul(M_persp2ortho_.mul(R_view_.mul(T_view_)));
        }

        Matrix4f getViewPort(const Vector2i &screen)
        {
            return Matrix4f{{screen[0] / 2.f, 0, 0, screen[0] / 2.f}, {0, screen[1] / 2.f, 0, screen[1] / 2.f}, {0, 0, 1 / 2.f, 1 / 2.f}, {0, 0, 0, 1}};
        }

        Vector3f get_lookat_dir()
        {
            return Vector3f{R_model_[0][2], R_model_[1][2], R_model_[2][2]}.normal();
        }

        float get_near()
        {
            return near_;
        }

        float get_far()
        {
            return far_;
        }
    };

    class LightComponent : public Component
    {
    private:
        Vector3f light_dir_;
        float light_intensity_;
        Vector3f light_color_;
        Vector3f specular_color_;
        Vector3f ambient_color_;

    public:
        LightComponent() : light_dir_(Vector3f{1.f, 0.f, 0.f}), light_intensity_(10.f), light_color_({1.f, 1.f, 1.f}), specular_color_({1.f, 1.f, 1.f}), ambient_color_({0.1f, 0.1f, 0.1f})
        {
        }

        LightComponent *set_light_dir(const Vector3f &light_dir)
        {
            light_dir_ = light_dir;
            return this;
        }
        LightComponent *set_intensity(float intensity)
        {
            light_intensity_ = intensity;
            return this;
        }

        Vector3f get_light_dir()
        {
            return light_dir_.normal();
        }
        float get_light_intensity()
        {
            return light_intensity_;
        }
        Vector3f get_light_color()
        {
            return light_color_;
        }
        Vector3f get_specular_color()
        {
            return specular_color_;
        }
        Vector3f get_ambient_color()
        {
            return ambient_color_;
        }
    };
}

#endif // ERER_CORE_COMPONENT_H_
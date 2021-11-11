#ifndef ERER_CORE_COMPONENT_H_
#define ERER_CORE_COMPONENT_H_

#include <iostream>
#include <vector>
#include <string>
#include <typeinfo>
#include <cmath>

#include "data_structure.hpp"
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
        void set_position(const Vector3f &pos)
        {
            T_model_[0][3] = pos[0];
            T_model_[1][3] = pos[1];
            T_model_[2][3] = pos[2];
        }
        void set_scala(const Vector3f &scala)
        {
            S_model_[0][0] = scala[0];
            S_model_[1][1] = scala[1];
            S_model_[2][2] = scala[2];
        }
        void set_rotation(const Vector3f &gaze, const Vector3f &up)
        {
            Vector3f w = gaze.normal();                          // x->w
            Vector3f u = up.normal();                            // y->u
            Vector3f v = Utils::cross_product_3D(w, u).normal(); // z->v
            for (int i = 0; i < 3; ++i)
            {
                R_model_[i][0] = w[i];
                R_model_[i][1] = u[i];
                R_model_[i][2] = v[i];
            }
        }
        Vector3f get_position()
        {
            return Vector3f{T_model_[0][3], T_model_[1][3], T_model_[1][3]};
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
        std::vector<Vector3f> positions_;
        std::vector<Vector3f> uvs_;
        std::vector<Vector3f> normals_;
        std::vector<Matrix3i> faces_;
        std::vector<Tensor<Tensor<Tensor<float, 3>, 3>, 3>> all_faces_;

    public:
        MeshComponent(const std::string &obj_filename) : Component()
        {
            Utils::load_obj_file(obj_filename, &positions_, &uvs_, &normals_, &faces_);
            all_faces_ = unpack_index(positions_, uvs_, normals_); // vertices uvs normals
        }

        std::vector<Tensor<Tensor<Tensor<float, 3>, 3>, 3>> unpack_index(const std::vector<Vector3f> &vertexes, const std::vector<Vector3f> &uvs, const std::vector<Vector3f> &normals)
        {
            std::vector<Tensor<Tensor<Tensor<float, 3>, 3>, 3>> ret;
            Tensor<Tensor<Tensor<float, 3>, 3>, 3> tmp;
            for (auto f : faces_)
            {
                for (size_t i = 0; i < 3; ++i)
                {
                    tmp[i][0] = vertexes[f[0][i]];
                    tmp[i][1] = uvs[f[1][i]];
                    tmp[i][2] = normals[f[2][i]];
                }
                ret.push_back(tmp);
            }
            return ret;
        }

        std::vector<Tensor<Tensor<Tensor<float, 3>, 3>, 3>> &get_all_faces()
        {
            return all_faces_;
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
        Matrix4f M_persp_;

    public:
        CameraComponent(float near = 0.1, float far = 100, float vertical_angle_of_view = 120, float horizontal_angle_of_view = 60)
            : Component(), render_buffer_(new unsigned char[Settings::WIDTH * Settings::HEIGHT * 3]), near_(near), far_(far), vertical_angle_of_view_(vertical_angle_of_view), horizontal_angle_of_view_(horizontal_angle_of_view)
        {
            M_persp_ = Matrix4f{{near_, 0, 0, 0}, {0, near_, 0, 0}, {0, 0, near_ + far_, -near_ * far_}, {0, 0, 1, 0}};
        }

        void lookat(const Vector3f &gaze, const Vector3f &up)
        {
            set_rotation(gaze, up); // set x y axis direction
            R_view_ = R_model_.transpose();
            T_view_ = T_model_;
            T_view_[0][3] *= -1;
            T_view_[1][3] *= -1;
            T_view_[2][3] *= -1;
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
            return M_persp_;
        }

        Matrix4f getVP()
        {
            return M_persp_.mul(R_view_.mul(T_view_));
        }

        Matrix4f getViewPort(const Vector2i &screen)
        {
            Matrix4f S = indentity<float, 4>();
            float f1 = static_cast<float>(screen[0] / (2 * near_ * std::tan(horizontal_angle_of_view_ / 2 / PI / 2)));
            float f2 = static_cast<float>(screen[1] / (2 * near_ * std::tan(vertical_angle_of_view_ / 2 / PI / 2)));
            S[0][0] = f1;
            S[1][1] = f2;
            Matrix4f T = indentity<float, 4>();
            T[0][3] = -Settings::WIDTH / 2;
            T[1][3] = -Settings::HEIGHT / 2;
            return T.mul(S);
        }
    };
}

#endif // ERER_CORE_COMPONENT_H_
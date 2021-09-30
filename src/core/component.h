#ifndef ERER_CORE_COMPONENT_H_
#define ERER_CORE_COMPONENT_H_

#include <iostream>
#include <vector>
#include <string>
#include <typeinfo>

#include "data_structure.hpp"
#include "../utils/loader.h"

namespace Core
{
    class Component
    {
    protected:
        static std::string eid_;

        Vector3f position_;
        Vector3f scale_;
        Vector4f rotate_;

    public:
        Component()
        {
            position_ = Vector3f();
            scale_ = Vector3f({1, 1, 1});
            rotate_ = Vector4f();
        }
        virtual ~Component() {} // virtual func is necessary, otherwise dynamic_cast(base->drived) will fail
        static std::string id()
        {
            return eid_;
        }
    };

    std::string Component::eid_; // static member variable must be redefined in global namespace again

    class MeshComponent : public Component
    {
    private:
        std::vector<Vector3f> vertices_;
        std::vector<Vector3f> uvs_;
        std::vector<Vector3f> normals_;
        std::vector<Matrix3i> faces_;

    public:
        MeshComponent(const std::string &obj_filename) : Component()
        {
            if (eid_.empty())
            {
                eid_ = typeid(*this).name();
                // eid_ = std::hash<std::string>{}(typeid(*this).name());
            }

            Utils::load_obj_file(obj_filename, &vertices_, &uvs_, &normals_, &faces_);
        }

        std::vector<Tensor<Matrix3f, 3>> get_all_faces()
        {
            std::vector<Tensor<Matrix3f, 3>> ret;
            Tensor<Matrix3f, 3> tmp;
            for (auto f : faces_)
            {
                for (int i = 0; i < 3; ++i)
                {
                    tmp[0][i] = vertices_[f[0][i]];
                    tmp[1][i] = uvs_[f[1][i]];
                    tmp[2][i] = normals_[f[2][i]];
                }
                ret.push_back(tmp);
            }
            return ret;
        }
    };

    class CameraComponent : public Component
    {
    private:
        float near_;
        float far_;
    public:
        CameraComponent() : Component()
        {
            if (eid_.empty())
            {
                eid_ = typeid(*this).name();
                // eid_ = std::hash<std::string>{}(typeid(*this).name());
            }
        }

        void lookat(const Vector3f &pos, const Vector3f &up)
        {
        }
    };
}

#endif // ERER_CORE_COMPONENT_H_
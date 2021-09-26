#ifndef ERER_CORE_COMPONENT_H_
#define ERER_CORE_COMPONENT_H_

#include <iostream>
#include <vector>
#include <string>
#include <typeinfo>

#include "data_structure.hpp"

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
        std::vector<Matrix3i> faces_;
        std::vector<Vector3f> vertices_;

    public:
        MeshComponent() : Component()
        {
            if (eid_.empty())
            {
                eid_ = typeid(*this).name();
                // eid_ = std::hash<std::string>{}(typeid(*this).name());
            }
        }
    };
}

#endif // ERER_CORE_COMPONENT_H_
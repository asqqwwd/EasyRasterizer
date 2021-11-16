#ifndef ERER_CORE_ENTITY_H_
#define ERER_CORE_ENTITY_H_

#include <iostream>
#include <vector>
#include <string>
#include <typeinfo>
#include <type_traits>

#include "component.h"

namespace Core
{
    class Entity
    {
    private:
        std::string eid_;
        std::vector<Component *> components_;

    public:
        Entity() = default;
        Entity(const std::string &eid) : eid_(eid)
        {
        }
        ~Entity()
        {
            eid_.clear();
            eid_.shrink_to_fit();
            components_.clear();
            components_.shrink_to_fit();
        }

        template <typename T>
        T* add_component(T *component)
        {
            components_.push_back(component);
            return component;
        }

        std::vector<Component *> &get_all_components()
        {
            return components_;
        }

        template <typename T>
        T *get_component()
        {
            static_assert(std::is_base_of<Component, T>::value,
                          "Template param must be component");

            for (auto p : components_)
            {
                if (typeid(T) == typeid(*p))
                {
                    return dynamic_cast<T *>(p);
                }
            }
            return nullptr;
        }

        std::string id()
        {
            return eid_;
        }
    };
}

#endif // ERER_CORE_ENTITY_H_
#ifndef ERER_CORE_SCENE_H_
#define ERER_CORE_SCENE_H_

#include <iostream>
#include <vector>
#include <map>
#include <string>

#include "entity.h"
#include "component.h"

namespace Core
{
    class Scene;
    std::map<std::string, Scene *> all_scenes;

    class Scene
    {
    private:
        std::string sid_;
        std::map<std::string, Entity *> entities_;

    public:
        Scene(const std::string &sid)
        {
            sid_ = sid; // deep copy
        }
        ~Scene()
        {
            sid_.clear();
            sid_.shrink_to_fit(); // unique method of string and vector after c++ 11
            entities_.clear();    // it will call destructor of each item in map
        }

        Entity *add_entity(Entity *entity)
        {
            if (entities_.find(entity->id()) != entities_.end())
            {
                std::clog << "[" << entity->id() + "] is existed" << std::endl;
                return entity;
            }
            entities_[entity->id()] = entity;
            return entity;
            // entities_.insert(std::pair<std::string, Entity *>(entity->id(), entity));
        }

        void delete_entity(const std::string &eid)
        {
            if (!entities_.erase(eid))
            {
                std::clog << "[" << eid + "] is not exist" << std::endl;
            }
        }

        Entity *get_entity(const std::string eid)
        {
            auto iter = entities_.find(eid);

            if (iter != entities_.end())
            {
                return iter->second;
            }
            std::clog << "Can't find entity [" << eid << "]" << std::endl;
            return nullptr;
        }

        std::vector<Entity *> get_all_entities()
        {
            std::vector<Entity *> ret;
            for (auto iter = entities_.begin(); iter != entities_.end(); iter++)
            {
                ret.push_back(iter->second);
            }
            return ret;
        }

        template <typename T>
        std::vector<T *> get_all_components()
        {
            static_assert(std::is_base_of<Component, T>::value,
                          "Template param must be component");

            std::vector<T *> ret;
            T *tmp;
            for (auto iter = entities_.begin(); iter != entities_.end(); iter++)
            {
                tmp = iter->second->get_component<T>(); // it will return nullptr if not find component
                if (tmp != nullptr)
                {
                    ret.push_back(tmp);
                }
            }
            return ret;
        }

        std::string id()
        {
            return sid_;
        }
    };

    Scene *current_scene = nullptr;
    void cd_to_scene(Scene *scene)
    {
        std::clog << "Change to scene [" << scene->id() << "]" << std::endl;
        current_scene = scene;
    }
    void add_entity(Entity *entity)
    {
        if (!current_scene)
        {
            std::cerr << "Current scene is empty" << std::endl;
            exit(-1);
        }
        current_scene->add_entity(entity);
    }
    void delete_entity(const std::string &eid)
    {
        if (!current_scene)
        {
            std::cerr << "Current scene is empty" << std::endl;
            exit(-1);
        }
        current_scene->delete_entity(eid);
    }
    Entity *get_entity(const std::string &eid)
    {
        if (!current_scene)
        {
            std::cerr << "Current scene is empty" << std::endl;
            exit(-1);
        }
        return current_scene->get_entity(eid);
    }
    std::vector<Entity *> get_all_entities()
    {
        if (!current_scene)
        {
            std::cerr << "Current scene is empty" << std::endl;
            exit(-1);
        }
        return current_scene->get_all_entities();
    }
    template <typename T>
    std::vector<T *> get_all_components()
    {
        if (!current_scene)
        {
            std::cerr << "Current scene is empty" << std::endl;
            exit(-1);
        }
        return current_scene->get_all_components<T>();
    }

    class SceneFactory
    {
    public:
        static Scene *build_default_scene()
        {
            Scene *scene = new Scene("Default");

            scene->add_entity(new Entity("HeadObj"))->add_component(new MeshComponent())->load_vertexes("../obj/helmet/helmet.obj")->load_albedo_texture("../obj/helmet/helmet_diffuse.tga");

            Vector3f main_camera_pos{0, 0, 2};
            scene->add_entity(new Entity("MainCamera"))->add_component(new CameraComponent(1.f, 10.f, 90.f, 90.f))->set_position(main_camera_pos);
            scene->get_entity("MainCamera")->get_component<CameraComponent>()->lookat(-1 * main_camera_pos, Vector3f{0, 1, 0});

            Vector3f main_light_pos{2, 1, 5};
            scene->add_entity(new Entity("MainLight"))->add_component(new LightComponent())->set_light_dir(main_light_pos - Vector3f{0, 0, 0})->set_intensity(30.f)->set_position(main_light_pos);

            return scene;
        }
    };
}

#endif // ERER_CORE_SCENE_H_
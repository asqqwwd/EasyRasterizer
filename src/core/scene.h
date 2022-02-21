#ifndef ERER_CORE_SCENE_H_
#define ERER_CORE_SCENE_H_

#include <iostream>
#include <vector>
#include <map>
#include <string>

#include "entity.h"
#include "component.h"
#include "../utils/convert.h"
#include "../utils/tgaimage.h"

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

            // scene
            //     ->add_entity(new Entity("TestObj"))                                                                     //
            //     ->add_component(new MeshComponent())                                                                    //
            //     ->load_vertexes("../../obj/cube.obj")                                                                   //
            //     ->set_albedo_texture(Utils::convert_TGAImage_to_CoreImage(Utils::TGAImage("../../obj/colormap24.tga"))) //
            //     ->set_scala(Vector3f{1, 1, 1})                                                                          //
            //     ->set_position(Vector3f{0, 0.5f, 0});

            // scene
            //     ->add_entity(new Entity("HelmetObj"))                                                                              //
            //     ->add_component(new MeshComponent())                                                                               //
            //     ->load_vertexes("../../obj/helmet/helmet.obj")                                                                     //
            //     ->set_albedo_texture(Utils::convert_TGAImage_to_CoreImage(Utils::TGAImage("../../obj/helmet/helmet_diffuse.tga"))) //
            //     ->set_scala(Vector3f{0.8, 0.8, 0.8})                                                                               //
            //     ->set_position(Vector3f{0, 0, 0});                                                                                 //

            // scene
            //     ->add_entity(new Entity("MaryObj"))                                                                                    //
            //     ->add_component(new MeshComponent())                                                                                   //
            //     ->load_vertexes("../../obj/marry/Marry.obj")                                                                           //
            //     ->set_albedo_texture(Utils::convert_TGAImage_to_CoreImage(Utils::TGAImage("../../obj/marry/MC003_Kozakura_Mari.tga"))) //
            //     ->set_scala(Vector3f{0.5, 0.5, 0.5})                                                                                   //
            //     ->set_position(Vector3f{0, -1, 0});                                                                                    //

            scene
                ->add_entity(new Entity("FloorObj"))                                                                             //
                ->add_component(new MeshComponent())                                                                             //
                ->load_vertexes("../../obj/floor/floor.obj")                                                                     //
                ->set_albedo_texture(Utils::convert_TGAImage_to_CoreImage(Utils::TGAImage("../../obj/floor/floor_diffuse.tga"))) //
                ->set_scala(Vector3f{1, 1, 1})                                                                                   //
                ->set_position(Vector3f{0, 0, 0});                                                                               //

            Vector3f main_camera_pos{0.f, 1.5f, 1.5f};
            scene
                ->add_entity(new Entity("MainCamera"))                      //
                ->add_component(new CameraComponent())                      //
                ->lookat_with_fixed_up(Vector3f{0, 0, 0} - main_camera_pos) //
                ->set_position(main_camera_pos);                            //

            Vector3f main_light_pos{5, 5, 5};
            scene
                ->add_entity(new Entity("MainLight"))               //
                ->add_component(new LightComponent())               //
                ->set_light_dir(Vector3f{0, 0, 0} - main_light_pos) //
                ->set_intensity(1.0f)                               //
                ->set_position(main_light_pos);                     //

            return scene;
        }
    };
}

#endif // ERER_CORE_SCENE_H_
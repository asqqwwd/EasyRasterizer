#ifndef ERER_CORE_SYSTEM_H_
#define ERER_CORE_SYSTEM_H_

#include <iostream>
#include <vector>
#include <stdlib.h>

#include "scene.h"

namespace Core
{
    extern Scene *current_scene;

    class System
    {
    private:
    public:
        virtual void update() = 0; // pure virtual func
    };

    std::vector<System *> all_systems;

    class RasterizeSystem : public System
    {
    private:
    public:
        RasterizeSystem() : System()
        {
            all_systems.push_back(this);
        }
        void update()
        {
            if (!current_scene)
            {
                std::cerr << "Current scene is empty" << std::endl;
                exit(-1);
            }
            for(auto compt:get_all_components<MeshComponent>()){
            }
        }
    };

}

#endif // ERER_CORE_SYSTEM_H_
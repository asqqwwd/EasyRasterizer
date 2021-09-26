#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <typeinfo>
#include <cassert>
#include <initializer_list>
#include <memory>

#include "core/data_structure.hpp"
#include "core/system.h"
#include "core/scene.h"
#include "core/entity.h"
#include "core/component.h"

using namespace std;
// using namespace Core;

int main(int argc, char **argv)
{
    new Core::RasterizeSystem();
    Core::cd_to_scene(new Core::Scene("Default"));

    Core::add_entity(new Core::Entity("Obj1"));
    Core::get_entity("Obj1")->add_component(new Core::MeshComponent());
    for (auto sys : Core::all_systems)
    {
        sys->update();
    }
}
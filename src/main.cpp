#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <typeinfo>
#include <cassert>
#include <initializer_list>
#include <memory>
#include <cstdlib>
#include <string.h>

#include <GL/glut.h>

#include "settings.h"
#include "core/data_structure.hpp"
#include "core/system.h"
#include "core/scene.h"
#include "core/entity.h"
#include "core/component.h"

using namespace std;

void display(void)
{
    for (auto sys : Core::all_systems)
    {
        sys->update();
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawPixels(Settings::WIDTH, Settings::HEIGHT, GL_BGR_EXT, GL_UNSIGNED_BYTE, Core::get_entity("MainCamera")->get_component<Core::CameraComponent>()->get_render_image());
    glutSwapBuffers(); // swap double buffer
}

void window_init(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB); // double buffer
    glutInitWindowSize(Settings::WIDTH, Settings::HEIGHT);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("ERer");
}

void game_init()
{
    new Core::RasterizeSystem();
    Core::cd_to_scene(new Core::Scene("Default"));

    Core::add_entity(new Core::Entity("HeadObj"));
    Core::get_entity("HeadObj")->add_component(new Core::MeshComponent("D:/Pro/CppPro/EasyRasterizer/obj/african_head.obj")); // Windows

    Core::add_entity(new Core::Entity("MainCamera"));
    Core::get_entity("MainCamera")->add_component(new Core::CameraComponent(0.1f, 100.f, 120.f, 90.f));
    Core::get_entity("MainCamera")->get_component<Core::CameraComponent>()->lookat(Core::Vector3f{1, 0, 0}, Core::Vector3f{0, 1, 0});
}

int main(int argc, char **argv)
{
    window_init(argc, argv);
    game_init();

    glutDisplayFunc(display);
    glutMainLoop(); // main loop

    return 0;
}
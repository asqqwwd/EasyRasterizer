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
    glDrawPixels(Settings::WIDTH, Settings::HEIGHT, GL_BGR_EXT, GL_UNSIGNED_BYTE, Core::render_image);
    glutSwapBuffers(); // swap double buffer
}

void window_init(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB); // double buffer
    glutInitWindowSize(Settings::WIDTH, Settings::HEIGHT);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("ERer");
    glutDisplayFunc(display);
}

void game_init()
{
    new Core::RasterizeSystem();
    Core::cd_to_scene(new Core::Scene("Default"));

    Core::add_entity(new Core::Entity("Obj1"));
    Core::get_entity("Obj1")->add_component(new Core::MeshComponent("../obj/african_head.obj"));
}

int main(int argc, char **argv)
{
    window_init(argc, argv);
    game_init();
    glutMainLoop(); // main loop

    return 0;
}
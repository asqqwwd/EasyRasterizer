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
#include "core/time.h"
#include "utils/loader.h"

#include <typeinfo>

using namespace std;

void display(void)
{
    Core::Time::clock();
    for (auto sys : Core::all_systems)
    {
        sys->update();
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawPixels(Settings::WIDTH, Settings::HEIGHT, GL_BGR_EXT, GL_UNSIGNED_BYTE, Core::get_entity("MainCamera")->get_component<Core::CameraComponent>()->get_render_buffer());
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
    // new Core::MotionSystem();
    Core::cd_to_scene(Core::SceneFactory::build_default_scene());
}

int main(int argc, char **argv)
{
    window_init(argc, argv);
    game_init();

    glutDisplayFunc(display);
    // glutIdleFunc(display); // force flush when idle
    glutMainLoop(); // main loop

    return 0;
}
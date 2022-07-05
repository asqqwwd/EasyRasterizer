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
    Core::Time::clock();

    // Core::Image<float> dp_img = Core::get_entity("MainLight")->get_component<Core::CameraComponent>()->get_depth_buffer();
    // Utils::TGAImage dp_img1 = Utils::convert_CoreImage_to_TGAImage(dp_img);
    // dp_img1.write_tga_file("./dp.tga");

    // Utils::TGAImage color_img = Utils::convert_raw_data_to_TGAImage(Core::get_entity("MainCamera")->get_component<Core::CameraComponent>()->get_color_buffer(), Settings::WIDTH, Settings::HEIGHT, 3);
    // color_img.write_tga_file("color.tga");

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawPixels(Settings::WIDTH, Settings::HEIGHT, GL_BGR_EXT, GL_UNSIGNED_BYTE, Core::get_entity("MainCamera")->get_component<Core::CameraComponent>()->get_color_buffer());
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
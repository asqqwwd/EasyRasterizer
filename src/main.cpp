#include <opencv2/opencv.hpp> // opencv全部功能
// #include <opencv2/core.hpp>  // opencv基本功能
#include <iostream> // 查找顺序：1.gcc指定目录 2.系统path
#include <string>
// #include <vector>
// #include <cmath>
#include <algorithm>
#include "settings.h"
#include "./utils/access/model.h" // 查找顺序：1.当前目录 2.vs设置c++ include/gcc -I指定目录 3.系统path

using namespace cv;

// auto clipFunc = [](int x, int minX, int maxX) -> const int
// {
//     return x < minX ? minX : (x >= maxX ? maxX - 1 : x);
// };
template <typename T>
T cross(T v1, T v2)
{
    return T(v1[1] * v2[2] - v1[2] * v2[1], v1[2] * v2[0] - v1[0] * v2[2], v1[0] * v2[1] - v1[1] * v2[0]);
}

void Line(int x0, int y0, int x1, int y1, Mat &img, Vec3b color)
{
    bool steep = std::abs(x0 - x1) > std::abs(y0 - y1) ? false : true;

    if (steep)
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }

    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    int dyDirect = dy > 0 ? 1 : -1;
    int derror = 2 * std::abs(dy);
    int error = 0;
    int y = y0;
    for (int x = x0; x <= x1; x++)
    {
        if (steep)
        {
            img.at<Vec3b>(y, x) = color;
        }
        else
        {
            img.at<Vec3b>(x, y) = color;
        }
        error += derror;
        if (error > dx)
        {
            y += dyDirect;
            error -= 2 * dx;
        }
    }
}

// 此方法未对多个点重合的情况进行处理
Vec3f GetBarycentricByArea(Vec2i *pts, Vec2i pt)
{
    float alpha = (-1. * (pt[0] - pts[1][0]) * (pts[2][1] - pts[1][1]) + (pt[1] - pts[1][1]) * (pts[2][0] - pts[1][0])) / (-(pts[0][0] - pts[1][0]) * (pts[2][1] - pts[1][1]) + (pts[0][1] - pts[1][1]) * (pts[2][0] - pts[1][0]));
    float beta = (-1. * (pt[0] - pts[2][0]) * (pts[0][1] - pts[2][1]) + (pt[1] - pts[2][1]) * (pts[0][0] - pts[2][0])) / (-(pts[1][0] - pts[2][0]) * (pts[0][1] - pts[2][1]) + (pts[1][1] - pts[2][1]) * (pts[0][0] - pts[2][0]));
    float gamma = 1 - alpha - beta;
    return Vec3f(alpha, beta, gamma);
}

// 已解决多个点重合问题
Vec3f GetBarycentricByVector(Vec2i *pts, Vec2i pt)
{
    Vec3i uv1 = cross(Vec3i(pts[0][0] - pts[1][0], pts[0][0] - pts[2][0], pt[0] - pts[0][0]), Vec3i(pts[0][1] - pts[1][1], pts[0][1] - pts[2][1], pt[1] - pts[0][1]));
    if (uv1[2] == 0)
    {
        return Vec3f(-1, -1, -1);
    }
    return Vec3f(1 - 1. * (uv1[0] + uv1[1]) / uv1[2], 1. * uv1[0] / uv1[2], 1. * uv1[1] / uv1[2]);
}

void Triangle(Vec2i *pts, Mat &img, Vec3b color)
{
    // 计算包围盒
    Vec2i bboxMin(std::max(0, std::min({pts[0][0], pts[1][0], pts[2][0]})), std::max(0, std::min({pts[0][1], pts[1][1], pts[2][1]})));
    Vec2i bboxMax(std::min(img.cols - 1, std::max({pts[0][0], pts[1][0], pts[2][0]})), std::min(img.rows - 1, std::max({pts[0][1], pts[1][1], pts[2][1]})));
    // 根据重心坐标计算颜色进行填充
    Vec2i pt;
    for (pt[0] = bboxMin[0]; pt[0] < bboxMax[0]; pt[0]++)
    {
        for (pt[1] = bboxMin[1]; pt[1] < bboxMax[1]; pt[1]++)
        {
            // Vec3f bc = GetBarycentricByArea(pts, pt);
            Vec3f bc = GetBarycentricByVector(pts, pt);
            if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0)
            {
                continue;
            }
            img.at<Vec3b>(pt[0], pt[1]) = color;
        }
    }
}

int main(int argc, char **argv)
{
    std::cout << argv[0] << std::endl;

    Mat img(Settings::WIDTH, Settings::HEIGHT, CV_8UC3, Scalar(0, 0, 0));
    Vec2i pts[3] = {Vec2i(1, 1), Vec2i(1, 1), Vec2i(1, 1)};

    Model model("D:/Pro/CppPro/Rasterize/obj/african_head.obj"); // 直接运行时这里的工作目录是运行该exe程序的终端当前目录，调试时工作目录会发生改变
    Vec3b white(255, 255, 255);
    for (int i = 0; i < model.nfaces(); i++)
    {
        // std::vector<int> face = model.face(i);
        // for (int j = 0; j < 3; j++)
        // {
        //     Vec3f v0 = model.vert(face[j]);
        //     Vec3f v1 = model.vert(face[(j + 1) % 3]);
        //     // 先将[-1,1]归一化到[0,1]。这里只使用了x,y坐标，直接投影到x,y平面
        //     int x0 = (v0[0] + 1.) * img.cols / 2.;
        //     int y0 = (v0[1] + 1.) * img.rows / 2.;
        //     int x1 = (v1[0] + 1.) * img.cols / 2.;
        //     int y1 = (v1[1] + 1.) * img.rows / 2.;
        //     Line(x0, y0, x1, y1, img, white);
        // }
        Vec2i *pts = new Vec2i[3];
        std::vector<int> face = model.face(i);
        for (int j = 0; j < 3; j++)
        {
            Vec3f v = model.vert(face[j]);
            pts[j][0] = (v[0] + 1.) * img.cols / 2.;
            pts[j][1] = (v[1] + 1.) * img.rows / 2.;
        }
        Triangle(pts, img, white);
    }

    namedWindow("Test");
    transpose(img, img);
    flip(img, img, 0); // 0=沿横轴翻转，1=沿纵轴翻转，-1=0+1

    imshow("Test", img);
    waitKey(0);
    destroyWindow("Test");

    return 0;
}
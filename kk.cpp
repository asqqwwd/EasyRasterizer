// #include <opencv2/opencv.hpp> // opencv全部功能
// // #include <opencv2/core.hpp>  // opencv基本功能
// #include <iostream> // 查找顺序：1.gcc指定目录 2.系统path
// #include <string>
// #include <vector>
// #include <cmath>
// #include <algorithm>
// #include "model.h" // 查找顺序：1.main当前目录及其子目录 2.vs设置c++ include/gcc -I指定目录 3.系统path

// // using namespace std;
// using namespace cv;

// const size_t WIDTH = 700;  // 水平方向长度
// const size_t HEIGHT = 700; // 垂直方向长度
// auto clipFunc = [](int x, int minX, int maxX) -> const int
// {
//     return x < minX ? minX : (x >= maxX ? maxX - 1 : x);
// };

// bool isInRange(int x, int minX, int maxX)
// {
//     return x < minX ? minX : (x >= maxX ? maxX - 1 : x);
// }

// void line(int x0, int y0, int x1, int y1, Mat &img, Vec3b color)
// {
//     x0 = clipFunc(x0, 0, WIDTH);
//     y0 = clipFunc(y0, 0, HEIGHT);
//     x1 = clipFunc(x1, 0, WIDTH);
//     y1 = clipFunc(y1, 0, HEIGHT);

//     bool steep = std::abs(x0 - x1) > std::abs(y0 - y1) ? false : true;

//     if (steep)
//     {
//         std::swap(x0, y0);
//         std::swap(x1, y1);
//     }

//     if (x0 > x1)
//     {
//         std::swap(x0, x1);
//         std::swap(y0, y1);
//     }

//     int dx = x1 - x0;
//     int dy = y1 - y0;
//     int dyDirect = dy > 0 ? 1 : -1;
//     int derror = 2 * std::abs(dy);
//     int error = 0;
//     int y = y0;
//     for (int x = x0; x <= x1; x++)
//     {
//         if (steep)
//         {
//             img.at<Vec3b>(y, x) = color;
//         }
//         else
//         {
//             img.at<Vec3b>(x, y) = color;
//         }
//         error += derror;
//         if (error > dx)
//         {
//             y += dyDirect;
//             error -= 2 * dx;
//         }
//     }
// }

// // int main(int argc, char **argv)
// // {
// //     Mat img(WIDTH, HEIGHT, CV_8UC3, Scalar(0, 0, 0));

// //     Model model("./../../obj/african_head.obj");
// //     Vec3b white(255, 255, 255);
// //     for (int i = 0; i < model.nfaces(); i++)
// //     {
// //         std::vector<int> face = model.face(i);
// //         for (int j = 0; j < 3; j++)
// //         {
// //             Vec3f v0 = model.vert(face[j]);
// //             Vec3f v1 = model.vert(face[(j + 1) % 3]);
// //             // 先将[-1,1]归一化到[0,1]。这里只使用了x,y坐标，直接投影到x,y平面
// //             int x0 = (v0[0] + 1.) * WIDTH / 2.;
// //             int y0 = (v0[1] + 1.) * HEIGHT / 2.;
// //             int x1 = (v1[0] + 2.) * WIDTH / 2.;
// //             int y1 = (v1[1] + 2.) * HEIGHT / 2.;
// //             line(x0, y0, x1, y1, img, white);
// //         }
// //     }

// //     namedWindow("Test");
// //     transpose(img, img);
// //     flip(img, img, 0); // 0=沿横轴翻转，1=沿纵轴翻转，-1=0+1

// //     imshow("Test", img);
// //     waitKey(0);
// //     destroyWindow("Test");

// //     return 0;
// // }
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

namespace
{
    Model::Model(const char *filename) : verts_(), faces_()
    {
        std::ifstream in;
        in.open(filename, std::ifstream::in);
        if (in.fail())
        {
            std::cout << "Can't find obj file!" << std::endl;
            return;
        }
        std::string line;
        while (!in.eof())
        {
            std::getline(in, line);
            std::istringstream iss(line.c_str()); // C++风格串流输入
            char trash;
            if (!line.compare(0, 2, "v "))
            {
                iss >> trash; // 读取v 0.1 0.2 0.3中的v到trash中，遇到空格停止
                cv::Vec3f v;
                for (int i = 0; i < 3; i++)
                    iss >> v[i]; // 读取0.1 0.2 0.3到v[i]中
                verts_.push_back(v);
            }
            else if (!line.compare(0, 2, "f "))
            {
                std::vector<int> f;
                int itrash, idx;
                iss >> trash; // 读取f 1/1/1 5/10/1 8/14/6中的f到trash中，这里1 5 8才是顶点索引，不是1 1 1
                while (iss >> idx >> trash >> itrash >> trash >> itrash)
                {
                    idx--; // in wavefront obj all indices start at 1, not zero
                    f.push_back(idx);
                }
                faces_.push_back(f);
            }
        }
        std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << std::endl;
    }

    Model::~Model()
    {
    }

    int Model::nverts()
    {
        return (int)verts_.size();
    }

    int Model::nfaces()
    {
        return (int)faces_.size();
    }

    std::vector<int> Model::face(int idx)
    {
        return faces_[idx];
    }

    cv::Vec3f Model::vert(int i)
    {
        return verts_[i];
    }
}

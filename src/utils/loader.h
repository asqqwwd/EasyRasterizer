#ifndef ERER_UTILS_LOADER_H_
#define ERER_UTILS_LOADER_H_

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "../core/data_structure.hpp"

namespace Utils
{
    void load_obj_file(const std::string filename, std::vector<Core::Vector3f> *vertices_p, std::vector<Core::Vector3f> *uvs_p, std::vector<Core::Vector3f> *normals_p, std::vector<Core::Matrix3i> *faces_p)
    {
        std::ifstream in;
        in.open(filename, std::ifstream::in);
        if (in.fail())
        {
            std::cout << "Can't find obj file!" << std::endl;
            return;
        }

        std::string line;
        char trash;
        float value;
        Core::Vector3f v;
        Core::Matrix3i m;
        while (!in.eof())
        {
            std::getline(in, line);
            std::istringstream iss(line.c_str()); // C++ style stream input
            if (!line.compare(0, 2, "v "))
            {
                iss >> trash; // read {v } in {v 0.1 0.2 0.3} to trash, stopping while encounter space charactor

                for (int i = 0; i < 3; i++)
                {
                    iss >> value;
                    v[i] = value;
                } // read {0.1 0.2 0.3} to v[i]
                vertices_p->push_back(v);
            }
            else if (!line.compare(0, 3, "vt "))
            {
                iss >> trash >> trash; // read {vt } in {vt 0.1 0.2 0} to trash, stopping while encounter space charactor
                for (int i = 0; i < 2; i++)
                {
                    iss >> value;
                    v[i] = value - static_cast<int>(value);
                }
                uvs_p->push_back(v);
            }
            else if (!line.compare(0, 3, "vn "))
            {
                iss >> trash >> trash; // read {vn } in {vn 0.1 0.2 0.3} to trash, stopping while encounter space charactor
                for (int i = 0; i < 3; i++)
                {
                    iss >> value;
                    v[i] = value;
                }
                normals_p->push_back(v);
            }
            else if (!line.compare(0, 2, "f "))
            {
                int vert_idx, uv_idx, normal_idx, i = 0;
                iss >> trash; // read {f} in {f 1/1/1 5/10/1 8/14/6} to trash, 1 5 8 is index of vertex instead of 1 1 1
                while (iss >> vert_idx >> trash >> uv_idx >> trash >> normal_idx)
                {
                    vert_idx--, uv_idx--, normal_idx--; // in wavefront obj all indices start at 1, not zero
                    m[0][i] = vert_idx;
                    m[1][i] = uv_idx;
                    m[2][i] = normal_idx;
                    ++i;
                }
                faces_p->push_back(m);
            }
        }
    }
}

#endif // ERER_UTILS_LOADER_H_
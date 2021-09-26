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
    void load_obj_file(const std::string filename, std::vector<Core::Vector3f> *vertices_p, std::vector<Core::Vector3f> *uvs_p, std::vector<Core::Vector3f> *normals_p, std::vector<Core::Vector3i> *faces_p)
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
            std::istringstream iss(line.c_str()); // C++ style stream input
            char trash;
            if (!line.compare(0, 2, "v "))
            {
                iss >> trash; // read {v } in {v 0.1 0.2 0.3} to trash, stopping while encounter space charactor
                Core::Vector3f v;
                float value;
                for (int i = 0; i < 3; i++)
                {
                    iss >> value;
                    v[i] = value;
                } // read {0.1 0.2 0.3} to v[i]
                vertices_p->push_back(v);
            }
            else if (!line.compare(0, 2, "vt "))
            {
                iss >> trash; // read {vt } in {vt 0.1 0.2 0} to trash, stopping while encounter space charactor
                Core::Vector3f v;
                float value;
                for (int i = 0; i < 3; i++)
                {
                    iss >> value;
                    v[i] = value;
                } // read {0.1 0.2 0.3} to v[i]
                vertices_p->push_back(v);
            }
            else if (!line.compare(0, 2, "vn "))
            {
            }
            else if (!line.compare(0, 2, "f "))
            {
                Core::Vector3i pos;
                Core::Vector3i uv;
                Core::Vector3i n;
                int vert_idx, uv_idx, normal_idx, i = 0;
                iss >> trash; // read {f} in {f 1/1/1 5/10/1 8/14/6} to trash, 1 5 8 is index of vertex instead of 1 1 1
                while (iss >> vert_idx >> trash >> uv_idx >> trash >> normal_idx)
                {
                    vert_idx--, uv_idx--, normal_idx--; // in wavefront obj all indices start at 1, not zero
                    pos[i++] = vert_idx;
                    uv[i++] = uv_idx;
                    n[i++] = normal_idx;
                } // read {1 5 8} in {1/1/1 5/10/1 8/14/6} to f
                // faces_p->push_back(u);
            }
        }
        std::clog << "v# " << vertices_p->size() << " f# " << faces_p->size() << std::endl;
    }
}

#endif // ERER_UTILS_LOADER_H_
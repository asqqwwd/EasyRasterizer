#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <typeinfo>
#include <cassert>
#include <initializer_list>
#include <memory>

#include "core/data_structure.h"
#include "core/entity.h"

int main(int argc, char **argv)
{
   Vector3f a(1.1f,2.1f,3.1f);
   Vector3f b(1.1f,2.1f,3.1f);
    a = a + b;
    std::cout << a[0] << " " << a[1] << " " << a[2] << std::endl;
}
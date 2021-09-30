# 指定最小支持版本，如果太小，会出现找不到具体cmake指令的错误
cmake_minimum_required(VERSION 3.10)

project(MainSolution)

set(CMAKE_CXX_STANDARD 17)

### 设置 ###
# set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)  # 编译设置为release
# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)  # 设置输出可执行文件路径
set (Is_Include_OpenCV FALSE)  # 不包含OpenCV库

### 包含OpenCV库 ###
if (Is_Include_OpenCV)
    # 自动搜索方式
    set(OpenCV_DIR "D:/ZH/opencv4.5.2/build")
    # 手动搜索方式
    # set(OpenCV_INCLUDE_DIRS ${OpenCV_DIR}/include/)
    # set(OpenCV_LIBRARIES_DIRS ${OpenCV_DIR}/x64/vc15/lib/)
    # set(OpenCV_LIBs ${OpenCV_DIR}/x64/vc15/bin/opencv_world345d.dll)
    # set(OpenCV_FOUND True)

    # 在OpenCV_DIR中找OpenCVConfig.cmake文件，找不到则终止编译
    find_package(OpenCV REQUIRED)  

    # 添加opencv头文件位置。头文件只有宏定义和函数定义，实际执行代码在静/动态库中
    include_directories(${OpenCV_INCLUDE_DIRS})
    # 指定要链接的库文件的路径，如果有find_package则不用该命令，因为可以自动找到链接路径
    # link_directories(${OpenCV_LIBRARY_DIRS}) 
endif() 

include_directories(/usr/include/cairo/)

# 在src目录下寻找所有源文件，加入SRC变量
# aux_source_directory("./src" SRC)  
file(GLOB_RECURSE SRC src/*.cpp src/*.h src/*.hpp)  # 递归遍历文件夹

# 添加源码到可执行项目。有main入口的可执行文件
add_executable(ERer ${SRC})
#  添加源码到静态/动态库项目。无main入口，可被其他项目调用
# add_library(ERer ${SRC})  

# target_link_libraries(ERer -lX11) # /usr/lib/x86_64-linux-gnu/libX11.so /usr/lib/x86_64-linux-gnu/libX11.a
# target_link_libraries(ERer -lcairo)
target_link_libraries(ERer -lglut -lGLU -lGL)

if (Is_Include_OpenCV)
    # 目标文件与库文件进行链接。静态库直接代码拷贝，动态库只做地址映射，dll文件不会自动复制过去，所以需要手动将opencv的dll添加到环境变量中
    target_link_libraries(ERer ${OpenCV_LIBS})
endif()

message("***** "  ${PROJECT_NAME}  " ***** "  ${SRC}  " *****")

# cmake -G "Visual Studio 16 2019" -A x64 -S ./ -B "build"
# cmake --build build --config Release
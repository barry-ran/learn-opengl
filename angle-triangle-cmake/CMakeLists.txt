cmake_minimum_required(VERSION 3.8)
set (project_name angle-triangle)
project(angle-triangle)

#find_library( OPENGLES3_LIBRARY GLESv2 "OpenGL ES v3.0 library")
#find_library( EGL_LIBRARY EGL "EGL 1.4 library" )

# 这里下载编译好的angle库 https://github.com/google/gfbuild-angle/releases
set (OPENGLES3_LIBRARY "C:/Users/Admin/Downloads/lib/libGLESv2.dll.lib")
set (EGL_LIBRARY "C:/Users/Admin/Downloads/lib/libEGL.dll.lib")
# 这里clone angle源码有头文件 https://github.com/google/angle
include_directories("C:/code/angle/include")

add_executable(${project_name} WIN32 main.cpp)

target_link_libraries(${project_name} ${OPENGLES3_LIBRARY} ${EGL_LIBRARY})

		
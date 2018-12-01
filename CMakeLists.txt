cmake_minimum_required(VERSION 3.11.0)
project(vplanet)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(vplanet
    src/gfx/System.cpp
    src/gfx/VulkanExt.cpp
    src/Application.cpp
    src/vplanet.cpp)
target_compile_features(vplanet PUBLIC cxx_std_14)
target_link_libraries(vplanet PUBLIC
    glfw
    Vulkan::Vulkan)

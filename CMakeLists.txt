cmake_minimum_required(VERSION 3.11.0)
project(vplanet)

if(WIN32)
    if (CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};../deps/glfw-msys")
    # elseif(MSVC AND "${CMAKE_SIZEOF_VOID_P}" STREQUAL 8)
    #     set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};../deps/boost-msvc-x64;../deps/glfw-msvc-x64;../deps/glm")
    # elseif(MSVC)
    #     set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};../deps/boost-msvc;../deps/glfw-msvc;../deps/glm")
    endif()
endif()


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

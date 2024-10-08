cmake_minimum_required(VERSION 3.30.0)
project(vplanet)

function(compile_spirv out_var)
    set(result)
    foreach(in_file ${ARGN})
        file(RELATIVE_PATH src_file ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${in_file})
        set(out_file ${PROJECT_BINARY_DIR}/${in_file}.spv)
        get_filename_component(out_dir ${out_file} DIRECTORY)
        file(MAKE_DIRECTORY ${out_dir})
        file(RELATIVE_PATH dst_file ${CMAKE_SOURCE_DIR} ${out_file})
        add_custom_command(
            OUTPUT ${out_file}
            COMMAND glslangValidator -V -o ${dst_file} ${src_file}
            DEPENDS ${in_file}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Building SPIR-V binary ${dst_file}"
            VERBATIM)
        list(APPEND result "${dst_file}")
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# set(Boost_USE_STATIC_LIBS "On")
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# GLM changed their library link target in a way that we can't really detect.
if(TARGET glm::glm)
  set(glm_library "glm::glm")
elseif(TARGET glm)
  set(glm_library "glm")
else()
  message(FATAL_ERROR "No glm link target found")
endif()

add_subdirectory(vendor/embed-resource)

compile_spirv(SPIRV_SHADERS
    src/gfx/shaders/ocean.vert
    src/gfx/shaders/ocean.frag
    src/gfx/shaders/terrain.vert
    src/gfx/shaders/terrain.frag)

embed_resources(EMBEDDED_SHADERS ${SPIRV_SHADERS})

add_executable(vplanet
    src/gfx/Commands.cpp
    src/gfx/DepthBuffer.cpp
    src/gfx/OceanPipeline.cpp
    src/gfx/Pipeline.cpp
    src/gfx/Renderer.cpp
    src/gfx/Swapchain.cpp
    src/gfx/System.cpp
    src/gfx/TerrainPipeline.cpp
    src/gfx/Uniforms.cpp
    src/gfx/VulkanExt.cpp
    src/Application.cpp
    src/Curve.cpp
    src/Models.cpp
    src/Noise.cpp
    src/Ocean.cpp
    src/Terrain.cpp
    src/vplanet.cpp
    ${EMBEDDED_SHADERS})
target_compile_features(vplanet PUBLIC cxx_std_14)
target_include_directories(vplanet PUBLIC vendor/embed-resource)
target_link_libraries(vplanet PUBLIC
    glfw
    ${glm_library}
    Vulkan::Vulkan)

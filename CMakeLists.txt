cmake_minimum_required(VERSION 4.0.0)
project(vplanet)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Add option to enable/disable C++ 20 module
option(ENABLE_CPP20_MODULE "Enable C++ 20 module support for Vulkan" OFF)

# Enable C++ module dependency scanning only if C++ 20 module is enabled
if(ENABLE_CPP20_MODULE)
  set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()

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
      COMMAND slangc ${src_file} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vs_main -entry fs_main -o ${out_file}
      DEPENDS ${in_file}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Building SPIR-V binary ${dst_file}"
      VERBATIM)
    list(APPEND result "${dst_file}")
  endforeach()
  set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(Vulkan 1.4.335 REQUIRED)
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
add_subdirectory(vendor/VulkanMemoryAllocator-3.3.0)

compile_spirv(SPIRV_SHADERS
  # src/gfx/shaders/ocean.vert
  # src/gfx/shaders/ocean.frag
  # src/gfx/shaders/terrain.vert
  # src/gfx/shaders/terrain.frag
)

embed_resources(EMBEDDED_SHADERS ${SPIRV_SHADERS})

# set up Vulkan C++ module only if enabled
if(ENABLE_CPP20_MODULE)
  add_library(VulkanCppModule)
  add_library(Vulkan::cppm ALIAS VulkanCppModule)

  target_compile_definitions(VulkanCppModule
    PUBLIC
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)
  target_include_directories(VulkanCppModule PUBLIC "${Vulkan_INCLUDE_DIR}")
  target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)

  set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

  # Add MSVC-specific compiler options for proper C++ module support
  if(MSVC)
    target_compile_options(VulkanCppModule PRIVATE
      /std:c++latest      # Use latest C++ standard for better module support
      /permissive-        # Standards conformance mode
      /Zc:__cplusplus     # Enable correct __cplusplus macro
      /EHsc               # Enable C++ exception handling
      /Zc:preprocessor    # Use conforming preprocessor
      /translateInclude)  # Automatically translate #include to import for standard library
  endif()

  target_sources(VulkanCppModule
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES
    BASE_DIRS
    "${Vulkan_INCLUDE_DIR}"
    FILES
    "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    "${Vulkan_INCLUDE_DIR}/vulkan/vulkan_video.cppm")

  # Add the vulkan.cppm file directly as a source file
  target_sources(VulkanCppModule
    PRIVATE
    "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    "${Vulkan_INCLUDE_DIR}/vulkan/vulkan_video.cppm")
else()
  # Create a dummy interface library when C++ 20 module is disabled
  add_library(VulkanCppModule INTERFACE)
  add_library(Vulkan::cppm ALIAS VulkanCppModule)
  target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
  target_compile_definitions(VulkanCppModule
    INTERFACE
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)
endif()

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
    src/VmaUsage.cpp
    src/vplanet.cpp
    ${EMBEDDED_SHADERS})
target_compile_features(vplanet PUBLIC cxx_std_23)
target_include_directories(vplanet PUBLIC vendor/embed-resource)

if(ENABLE_CPP20_MODULE)
  target_compile_definitions(vgraphplay PRIVATE USE_CPP20_MODULES=1)
endif()

target_link_libraries(vplanet PUBLIC
    glfw
    ${glm_library}
    Vulkan::cppm
    GPUOpen::VulkanMemoryAllocator)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set_source_files_properties(src/VmaUsage.cpp PROPERTIES COMPILE_OPTIONS "-w")
endif()

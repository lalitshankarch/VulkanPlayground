cmake_minimum_required(VERSION 3.25)
project(VulkanPlayground)
set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall)
endif()

find_package(Vulkan REQUIRED)

set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Disable Wayland support")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/spdlog)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/vk-bootstrap)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/glm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/tinyobjloader)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/VulkanMemoryAllocator/include)

link_libraries(glfw spdlog vk-bootstrap Vulkan::Vulkan)

add_executable(HelloTriangle src/HelloTriangle/main.cpp)
add_executable(HelloMeshTriangle src/HelloMeshTriangle/main.cpp)
add_executable(HelloMeshLoader src/HelloMeshLoader/main.cpp)
file(GLOB_RECURSE LVE_SOURCES "src/lve/*.cpp")
add_executable(lve ${LVE_SOURCES})

set_target_properties(HelloTriangle PROPERTIES WIN32_EXECUTABLE "$<$<CONFIG:Release>:TRUE>")
set_target_properties(HelloMeshTriangle PROPERTIES WIN32_EXECUTABLE "$<$<CONFIG:Release>:TRUE>")
set_target_properties(HelloMeshLoader PROPERTIES WIN32_EXECUTABLE "$<$<CONFIG:Release>:TRUE>")
cmake_minimum_required(VERSION 3.0.0)
project(vfs)

set (CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(vfs main.cpp)
target_link_libraries(vfs ${Vulkan_LIBRARIES} glfw )
target_compile_definitions(vfs PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=0)

file(COPY shaders DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

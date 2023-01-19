cmake_minimum_required(VERSION 3.20)
project(RenderEngine)

include(cmake/find_vkw.cmake)
include(cmake/find_vulkan.cmake)
set(CMAKE_CXX_STANDARD 20)

file(GLOB ENGINE_SOURCES include/RE/*.hpp source/*.cpp source/*.hpp)
add_library(RenderEngine SHARED ${ENGINE_SOURCES})

find_package(Boost 1.62 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(RenderEngine PUBLIC ${RENDERER_LIB})
target_include_directories(RenderEngine PUBLIC include)
set(RE_RUNTIME_DIR ${CMAKE_CURRENT_BINARY_DIR})
add_subdirectory(test)

install(TARGETS RenderEngine)
install(DIRECTORY include DESTINATION .)
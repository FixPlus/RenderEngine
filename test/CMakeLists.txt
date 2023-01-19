
option(ENABLE_TESTING "Build and run tests" OFF)
if (NOT ${ENABLE_TESTING})
    message(STATUS "Testing disabled")
endif ()

find_package(glfw3 3.3 REQUIRED)

file(GLOB TEST_COMMON_SOURCE common/*.h common/*.cpp)

add_library(TestCommon STATIC ${TEST_COMMON_SOURCE})

target_link_libraries(TestCommon glfw RenderEngine)
target_include_directories(TestCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

file(GLOB TESTS tests/*.cpp)

foreach (TEST ${TESTS})
    get_filename_component(TEST_NAME ${TEST} NAME_WE)
    add_executable(${TEST_NAME} ${TEST})
    target_link_libraries(${TEST_NAME} TestCommon)
    set_target_properties(${TEST_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_DEBUG ${RE_RUNTIME_DIR}
            RUNTIME_OUTPUT_DIRECTORY_RELEASE ${RE_RUNTIME_DIR}
            )
endforeach ()
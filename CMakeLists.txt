cmake_minimum_required(VERSION 3.5)
project(intrusive_list)

option(BUILD_INTRUSIVE_LIST_TESTS "Build ${PROJECT_NAME} tests" OFF)

set(CMAKE_CXX_STANDARD 11)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

if (BUILD_INTRUSIVE_LIST_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

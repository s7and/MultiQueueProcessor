cmake_minimum_required(VERSION 3.10.2)
project(MultiQueueProcessor)

find_package(Threads REQUIRED)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories( ${PROJECT_NAME} INTERFACE include/ )

include(CTest)
enable_testing()
add_subdirectory(test)
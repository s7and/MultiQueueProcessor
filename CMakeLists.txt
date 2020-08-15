cmake_minimum_required(VERSION 3.10.2)
project(MultiQueueProcessor)

find_package(Threads REQUIRED)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories( ${PROJECT_NAME} INTERFACE include/ )
target_link_libraries(${PROJECT_NAME} INTERFACE CONAN_PKG::boost)

include(CTest)
enable_testing()
add_subdirectory(test)
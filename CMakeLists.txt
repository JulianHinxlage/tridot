cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 20)

project(tridot)
file(GLOB_RECURSE SOURCES src/*.cpp)
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC src)
target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_DEBUG)

project(test-tridot)
add_executable(${PROJECT_NAME} test/test.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC tridot)

project(tridot)

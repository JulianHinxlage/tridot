cmake_minimum_required(VERSION 3.4)
set(CMAKE_CXX_STANDARD 20)

#required linux packages: libxinerama-dev libxcursor-dev libxi-dev
project(tridot)

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MTd")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

if(WIN32)
    set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
    set(CMAKE_DEBUG_POSTFIX "")
    set(CMAKE_IMPORT_LIBRARY_SUFFIX ".lib")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

file(GLOB_RECURSE SOURCES src/tridot/*.cpp)
file(GLOB IMGUI_SOURCES extern/imgui/*.cpp extern/imgui/backends/imgui_impl_glfw.cpp extern/imgui/backends/imgui_impl_opengl3.cpp)
file(GLOB IMGUIZMO_SOURCES extern/imguizmo/ImGuizmo.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${IMGUI_SOURCES} ${IMGUIZMO_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC src extern extern/imgui)
target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_DEBUG ECS_DEBUG)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TRI_DLL_EXPORT)
endif()

set(TRIDOT_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
set(CMAKE_BUILD_TYPE "Release")

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)


add_subdirectory(extern/bullet3)
target_include_directories(${PROJECT_NAME} PUBLIC extern/bullet3/src)
target_link_libraries(${PROJECT_NAME} BulletDynamics BulletCollision LinearMath)

set(CMAKE_BUILD_TYPE ${TRIDOT_BUILD_TYPE})
add_subdirectory(extern/yaml-cpp)
set(CMAKE_BUILD_TYPE "Release")

add_subdirectory(extern/glfw)
add_subdirectory(extern/glew/build/cmake)
add_subdirectory(extern/glm)
target_link_libraries(${PROJECT_NAME} glfw glm yaml-cpp)
target_include_directories(${PROJECT_NAME} PUBLIC extern/glm extern/glfw/include extern/glew/include extern/yaml-cpp/include)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} opengl32 glew_s imm32)
else()
    target_link_libraries(${PROJECT_NAME} GL glew pthread stdc++fs)
endif()

set(CMAKE_BUILD_TYPE ${TRIDOT_BUILD_TYPE})

if(NOT WIN32)
add_link_options(-no-pie)
endif()

project(blink)
add_library(${PROJECT_NAME} SHARED src/test/plugin/Blink.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC extern/glm src)
if(NOT WIN32)
    target_compile_options(${PROJECT_NAME} PUBLIC -fno-gnu-unique)
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC tridot)

project(editor)
file(GLOB_RECURSE SOURCES src/editor/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC tridot)
add_dependencies(${PROJECT_NAME} blink)

project(test)
file(GLOB_RECURSE SOURCES src/test/test.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC tridot)
project(tridot)

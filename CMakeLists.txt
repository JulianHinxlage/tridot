cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 20)

project(tridot)
file(GLOB_RECURSE SOURCES src/tridot/*.cpp)
file(GLOB IMGUI_SOURCES extern/imgui/*.cpp extern/imgui/backends/imgui_impl_glfw.cpp extern/imgui/backends/imgui_impl_opengl3.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${IMGUI_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC src extern extern/imgui)
target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_DEBUG ECS_DEBUG)

#required packages: libglfw3-dev libglew-dev libglm-dev pkg-config libbullet-dev
find_package(Bullet REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${BULLET_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${BULLET_LIBRARIES})

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${OPENGL_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${glm_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} pthread stdc++fs)

project(test-tridot)
add_executable(${PROJECT_NAME} src/test/test.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC tridot)

project(tridot)

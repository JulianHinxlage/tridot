cmake_minimum_required(VERSION 3.4)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_UNITY_BUILD ON)
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()


project(tridot)
file(GLOB_RECURSE SOURCES "src/core/*.cpp" "src/entity/*.cpp" "src/engine/*.cpp" "src/render/*.cpp")
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC src src)
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TRI_DLL_EXPORT)
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_DEBUG)
endif()

add_subdirectory(extern/glfw)
add_subdirectory(extern/glew/build/cmake)
add_subdirectory(extern/glm)
target_link_libraries(${PROJECT_NAME} glfw glm)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} opengl32 glew_s imm32)
else()
    target_link_libraries(${PROJECT_NAME} GL glew pthread stdc++fs)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC extern/glm extern/glfw/include extern/glew/include)
target_include_directories(${PROJECT_NAME} PUBLIC extern)



project(tridot-launcher)
file(GLOB_RECURSE SOURCES src/launcher/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC src src/launcher)
target_link_libraries(${PROJECT_NAME} PUBLIC tridot)

project(tridot)
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
install(TARGETS tridot-launcher tridot RUNTIME)
install(FILES res/config.txt RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/res)

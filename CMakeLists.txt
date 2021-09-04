cmake_minimum_required(VERSION 3.4)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_UNITY_BUILD ON)
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

project(tridot-core)
file(GLOB_RECURSE SOURCES src/core/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC src src/core)
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TRI_DLL_EXPORT)
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_DEBUG)
endif()

project(module1)
add_library(${PROJECT_NAME} SHARED src/modules/Module1.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} PUBLIC tridot-core)

project(module2)
add_library(${PROJECT_NAME} SHARED src/modules/Module2.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} PUBLIC tridot-core)

project(tridot-launcher)
file(GLOB_RECURSE SOURCES src/launcher/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC src src/launcher)
target_link_libraries(${PROJECT_NAME} PUBLIC tridot-core)
add_dependencies(${PROJECT_NAME} module1 module2)

project(tridot)

set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
install(TARGETS tridot-launcher tridot-core module1 module2 RUNTIME)
install(FILES res/config.txt RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/res)

cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 20)

set(TRIDOT_DIRECTORY "../..")

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
    set(CMAKE_DEBUG_POSTFIX "")
    set(CMAKE_RELWITHDEBINFO_POSTFIX "")
    set(CMAKE_IMPORT_LIBRARY_SUFFIX ".lib")
    set(USE_MSVC_RUNTIME_LIBRARY_DLL 1)
    set(CMAKE_WIN32_EXECUTABLE true)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    add_link_options(-no-pie)
endif()

add_subdirectory(${TRIDOT_DIRECTORY} ${CMAKE_CURRENT_BINARY_DIR}/tridot)


#Template
project(Template)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC ${TRIDOT_DIRECTORY}/src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE ${TRIDOT_DIRECTORY}/src src)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotEntity TridotEngine)

add_dependencies(TridotEditor ${PROJECT_NAME})
add_dependencies(TridotGame ${PROJECT_NAME})
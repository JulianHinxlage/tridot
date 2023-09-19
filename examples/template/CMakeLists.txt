cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 20)

set(SOLUTION_NAME "template")
set(ENGINE_DIR "../..")

#general settings
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

add_subdirectory(${ENGINE_DIR} ${ENGINE_DIR}/../build)

#Template
project(Template)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src ../../src)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotEntity TridotEngine TridotWindow)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${SOLUTION_NAME})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT TridotEngine)
set_property(TARGET TridotEngine PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build/Debug")
add_dependencies(TridotEngine Template)

project(${SOLUTION_NAME})
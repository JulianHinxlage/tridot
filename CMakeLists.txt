cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 20)

#general settings
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
    set(CMAKE_DEBUG_POSTFIX "")
    set(CMAKE_RELWITHDEBINFO_POSTFIX "")
    set(CMAKE_IMPORT_LIBRARY_SUFFIX ".lib")
    set(USE_MSVC_RUNTIME_LIBRARY_DLL 1)
    #set(CMAKE_WIN32_EXECUTABLE true)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    add_link_options(-no-pie)
endif()
#set(CMAKE_UNITY_BUILD ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_FOLDER Libs)

#Tridot Core
project(TridotCore)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/core/*.cpp)
if (NOT CMAKE_BUILD_TYPE MATCHES "Release")
    file(GLOB_RECURSE SOURCES2 CONFIGURE_DEPENDS extern/tracy/TracyClient.cpp)
endif()
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${SOURCES2})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC src src/core extern)
target_include_directories(${PROJECT_NAME} PRIVATE src/core)

if (NOT CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_DEBUG)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_RELEASE)
endif()

if (NOT CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_PROFILE_ENABLED TRACY_ENABLE TRI_DEBUG)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_RELEASE)
endif()


#Tridot Entity
project(TridotEntity)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/entity/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src/entity)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore)

#Tridot Window
project(TridotWindow)
file(GLOB IMGUI_SOURCES CONFIGURE_DEPENDS extern/imgui/*.cpp extern/imgui/misc/cpp/imgui_stdlib.cpp extern/imgui/backends/imgui_impl_glfw.cpp extern/imgui/backends/imgui_impl_opengl3.cpp)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/window/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${IMGUI_SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC src src/window)
add_subdirectory(extern/glm)
add_subdirectory(extern/glfw)
add_subdirectory(extern/glew/build/cmake)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} TridotCore glm glfw opengl32 glew_s imm32)
else()
    target_link_libraries(${PROJECT_NAME} TridotCore glm glfw GL glew pthread stdc++fs)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC extern extern/glm extern/glfw/include extern/glew/include extern/imgui)


#Tridot Engine
project(TridotEngine)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/engine/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC extern/glm)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PUBLIC glm TridotCore TridotEntity)


#Tridot DebugMenu
project(TridotDebugMenu)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/debugMenu/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src/debugMenu)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotWindow)

#Tridot Editor
project(TridotEditor)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/editor/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src/editor)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotWindow TridotEntity TridotEngine)

#Tridot Render
project(TridotRender)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/render/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src/render)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotWindow TridotEngine)

#Tridot Launcher
project(TridotLauncher)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/launcher/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC src src/launcher)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore)
add_dependencies(${PROJECT_NAME} TridotWindow TridotEntity TridotEditor TridotDebugMenu TridotEngine TridotRender)

#folders
set_target_properties(TridotCore TridotWindow TridotEntity TridotEngine TridotDebugMenu TridotRender TridotLauncher PROPERTIES FOLDER Tridot)
set_target_properties(glfw uninstall PROPERTIES FOLDER Libs)

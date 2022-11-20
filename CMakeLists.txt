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
    set(CMAKE_WIN32_EXECUTABLE true)
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

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_DEBUG)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_RELEASE)
endif()

if (NOT CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_PROFILE_ENABLED TRACY_ENABLE)
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
file(GLOB IMGUI_SOURCES CONFIGURE_DEPENDS extern/imgui/*.cpp extern/imgui/misc/cpp/imgui_stdlib.cpp extern/imgui/backends/imgui_impl_glfw.cpp extern/imgui/backends/imgui_impl_opengl3.cpp extern/imguizmo/ImGuizmo.cpp)
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
target_include_directories(${PROJECT_NAME} PUBLIC extern/glm extern/yaml-cpp/include)
target_include_directories(${PROJECT_NAME} PRIVATE src)
add_subdirectory(extern/yaml-cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC glm yaml-cpp TridotCore TridotEntity)



#Tridot DebugMenu
project(TridotDebugMenu)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/debugMenu/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src src/debugMenu)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotWindow TridotEngine)

#Tridot Editor Module
project(TridotEditorModule)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/editor/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src src/editor)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotWindow TridotEntity TridotEngine TridotRender TridotAnimation)

#Tridot Render
project(TridotRender)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/render/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src src/render)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotWindow TridotEngine)

#Tridot Physics
project(TridotPhysics)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/physics/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src src/physics extern/bullet3/src)
add_subdirectory(extern/bullet3)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotEngine BulletDynamics BulletCollision LinearMath)

#Tridot Animation
project(TridotAnimation)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/animation/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src src/animation)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotEntity TridotEngine)

#Tridot Animation Editor
project(TridotAnimationEditor)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/animationEditor/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src src/animationEditor)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotAnimation TridotEditorModule)

#Tridot Network
project(TridotNetwork)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/network/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src src/network)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotEntity TridotEngine)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

#Tridot Particle System
project(TridotParticleSystem)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/particleSystem/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src src/particleSystem)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotEntity TridotEngine)

#Tridot Audio
project(TridotAudio)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/audio/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src src/audio)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotEngine)
target_compile_definitions(${PROJECT_NAME} PRIVATE AL_LIBTYPE_STATIC)
set(LIBTYPE "STATIC")
add_subdirectory(extern/openal)
target_include_directories(${PROJECT_NAME} PRIVATE extern/openal/include)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenAL)

#Tridot Gameplay
project(TridotGameplay)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/gameplay/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src src/gameplay)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotEntity TridotEngine TridotWindow TridotPhysics)



#Tridot Editor Executable
project(TridotEditor)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/launch/editor/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC src src/launch/editor)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore)
add_dependencies(${PROJECT_NAME} TridotWindow TridotEntity TridotEditorModule TridotDebugMenu TridotEngine TridotRender TridotPhysics TridotAnimation TridotParticleSystem TridotAnimationEditor TridotGameplay TridotAudio TridotNetwork)

#Tridot Game Executable
project(TridotGame)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/launch/game/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC src src/launcher/game)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotWindow)
add_dependencies(${PROJECT_NAME} TridotWindow TridotEntity TridotDebugMenu TridotEngine TridotRender TridotPhysics TridotAnimation TridotParticleSystem TridotGameplay TridotAudio TridotNetwork)


project(TridotHost)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/launch/host/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC src src/launcher/host)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotWindow)
add_dependencies(${PROJECT_NAME} TridotWindow TridotEntity TridotDebugMenu TridotEngine TridotRender TridotPhysics TridotAnimation TridotParticleSystem TridotGameplay TridotAudio TridotNetwork)

project(TridotClient)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/launch/client/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC src src/launcher/client)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore TridotWindow)
add_dependencies(${PROJECT_NAME} TridotWindow TridotEntity TridotDebugMenu TridotEngine TridotRender TridotPhysics TridotAnimation TridotParticleSystem TridotGameplay TridotAudio TridotNetwork)

set(CMAKE_WIN32_EXECUTABLE false)
project(TridotServer)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/launch/server/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC src src/launcher/server)
target_link_libraries(${PROJECT_NAME} PUBLIC TridotCore)
set(CMAKE_WIN32_EXECUTABLE true)
add_dependencies(${PROJECT_NAME} TridotWindow TridotEntity TridotEngine TridotPhysics TridotAnimation TridotParticleSystem TridotGameplay TridotAudio TridotNetwork)


#Fat Game Binary
project(TridotGameFull)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS 
    src/core/*.cpp 
    src/entity/*.cpp 
    src/engine/*.cpp 
    src/window/*.cpp 
    src/render/*.cpp 
    src/debugMenu/*.cpp 
    src/physics/*.cpp 
    src/animation/*.cpp 
    src/particleSystem/*.cpp 
    src/audio/*.cpp 
    src/gameplay/*.cpp 
    src/launch/game/*.cpp
)

if (NOT CMAKE_BUILD_TYPE MATCHES "Release")
    file(GLOB_RECURSE SOURCES2 CONFIGURE_DEPENDS extern/tracy/TracyClient.cpp)
endif()
file(GLOB IMGUI_SOURCES CONFIGURE_DEPENDS extern/imgui/*.cpp extern/imgui/misc/cpp/imgui_stdlib.cpp extern/imgui/backends/imgui_impl_glfw.cpp extern/imgui/backends/imgui_impl_opengl3.cpp extern/imguizmo/ImGuizmo.cpp)
add_executable(${PROJECT_NAME} ${SOURCES} ${SOURCES2} ${IMGUI_SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC src src/core src/launche src/render src/window)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} glm glfw opengl32 glew_s yaml-cpp imm32)
else()
    target_link_libraries(${PROJECT_NAME} glm glfw GL glew yaml-cpp pthread stdc++fs)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC extern extern/glm extern/glfw/include extern/glew/include extern/imgui extern/yaml-cpp/include extern/bullet3/src extern/openal/include)
target_link_libraries(${PROJECT_NAME} BulletDynamics BulletCollision LinearMath OpenAL)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_DEBUG)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_RELEASE)
endif()

if (NOT CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRI_PROFILE_ENABLED TRACY_ENABLE)
endif()


#folders
set_target_properties(TridotCore TridotWindow TridotEntity TridotEngine TridotDebugMenu TridotRender TridotEditorModule TridotGameplay TridotParticleSystem TridotAnimation TridotAnimationEditor TridotAudio TridotNetwork TridotEditor TridotGame TridotGameFull PROPERTIES FOLDER Tridot)
set_target_properties(glfw uninstall PROPERTIES FOLDER Libs)

project(Tridot)
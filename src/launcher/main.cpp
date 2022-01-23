//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "engine/MainLoop.h"
#include "core/core.h"
#include "engine/AssetManager.h"
#include "entity/Scene.h"
#include "engine/Camera.h"
#include "render/Window.h"

using namespace tri;

class EditorOnly{};
TRI_REGISTER_TYPE(EditorOnly);

int main(int argc, char* argv[]) {
    env->assets->hotReloadEnabled = false;

    //remove editor only entities when a scene is loaded
    env->signals->sceneLoad.addCallback("launcher", [](Scene *scene){
        scene->view<EditorOnly>().each([&](EntityId id, EditorOnly &){
            scene->removeEntity(id);
        });
        scene->view<Camera>().each([&](EntityId id, Camera &cam){
            if(cam.isPrimary){
                cam.active = true;
            }
        });
    });

    MainLoop loop;
    loop.startup({ "config.txt", "../config.txt", "../../config.txt" });

    //wait to load all assets before displaying the scene
    //wait 30 seconds max
    Clock clock;
    while(clock.elapsed() < 30.0f) {
        if (env->assets->isLoadingInProcess() && env->window->isOpen()) {
            env->window->update();
            env->assets->update();
        }
        else {
            break;
        }
    }

    env->signals->sceneBegin.invoke(env->scene);

    loop.run();
    loop.shutdown();
}

#if TRI_WINDOWS
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
    return main(nCmdShow, (char**)lpCmdLine);
}
#endif

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "engine/MainLoop.h"
#include "core/core.h"
#include "engine/AssetManager.h"
#include "entity/Scene.h"
#include "engine/Camera.h"

using namespace tri;

class EditorOnly{};
TRI_REGISTER_TYPE(EditorOnly);

int main(int argc, char* argv[]) {
    env->assets->hotReloadEnabled = false;
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
    loop.startup("config.txt", "../res/config.txt");
    loop.run();
    loop.shutdown();
}

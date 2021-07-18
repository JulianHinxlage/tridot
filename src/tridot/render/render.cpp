//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "render.h"
#include "tridot/engine/ResourceManager.h"

namespace tridot{

    TRI_REGISTER_CALLBACK(){
        env->events->init.callbackOrder({"engine", "render"});
    }

    TRI_INIT_CALLBACK("render"){
        env->window = env->systems->addSystem<Window>();
        env->renderer = env->systems->addSystem<MeshRenderer>();
        env->pbRenderer = env->systems->addSystem<PBRenderer>();

        env->window->init(800, 600, "Window");
        env->renderer->init(env->resources->get<Shader>("shaders/mesh.glsl"));
        env->pbRenderer->init(env->resources->get<Shader>("shaders/PBR.glsl"));
    }

    TRI_UPDATE_CALLBACK("window"){
        env->window->update();
        if(!env->window->isOpen()){
            env->events->exit.invoke();
        }
    }

}
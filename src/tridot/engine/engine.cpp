//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "engine.h"

namespace tridot{

    TRI_INIT_CALLBACK("engine"){
        env->time = env->systems->addSystem<Time>();
        env->input = env->systems->addSystem<Input>();
        env->profiler = env->systems->addSystem<Profiler>();
        env->physics = env->systems->addSystem<Physics>();
        env->scene = env->systems->addSystem<Scene>();
        env->resources = env->systems->addSystem<ResourceManager>();

        env->time->init();
        env->physics->init();

        env->resources->addSearchDirectory("../res");
        env->resources->addSearchDirectory("plugins");
        env->resources->autoReload = true;
        env->resources->threadCount = 8;
        env->resources->update();

        env->events->sceneEnd.addCallback("physics", [](){
            env->scene->view<RigidBody>().each([](EntityId id, RigidBody &rb){
                env->physics->remove(rb);
            });
        });
    }

    TRI_UPDATE_CALLBACK("time"){
        TRI_PROFILE("time");
        env->time->update();
    }
    TRI_UPDATE_CALLBACK("input"){
        TRI_PROFILE("input");
        env->input->update();
    }
    TRI_UPDATE_CALLBACK("profiler"){
        TRI_PROFILE("profiler");
        env->profiler->end("total");
        env->profiler->update();
        env->profiler->begin("total");
    }
    TRI_UPDATE_CALLBACK("resources"){
        TRI_PROFILE("resources");
        env->resources->update();
    }
    TRI_UPDATE_CALLBACK("physics"){
        TRI_PROFILE("physics");
        {
            TRI_PROFILE("physics/step");
            env->physics->step(env->time->deltaTime);
        }
        {
            TRI_PROFILE("physics/update");
            env->scene->view<Transform, RigidBody, Collider>().each([](EntityId id, Transform &t, RigidBody &rb, Collider &c){
                env->physics->update(rb, t, c, id);
            });
        }
    }

}
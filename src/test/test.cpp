//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "engine/Transform.h"
#include "entity/Scene.h"
#include "engine/Time.h"

namespace tri{

    class Velocity {
    public:
        glm::vec3 speed;
    };

    TRI_REGISTER_COMPONENT(Velocity);
    TRI_REGISTER_MEMBER(Velocity, speed);

    class Test : public Module{
    public:
        virtual void update() override{
            env->scene->view<Transform, Velocity>().each([](Transform &t, Velocity &v){
               t.position += v.speed * env->time->deltaTime;
            });
        }
    };

    TRI_REGISTER_MODULE(Test);

}
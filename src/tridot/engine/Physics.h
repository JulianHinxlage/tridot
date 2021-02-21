//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_PHYSICS_H
#define TRIDOT_PHYSICS_H

#include "tridot/core/Ref.h"
#include <glm/glm.hpp>
#include <functional>

namespace tridot {

    class Transform{
    public:
        glm::vec3 position = {0, 0, 0};
        glm::vec3 scale = {1, 1, 1};
        glm::vec3 rotation = {0, 0, 0};
    };

    class RigidBody{
    public:
        glm::vec3 velocity = {0, 0, 0};
        glm::vec3 angular = {0, 0, 0};
        float mass = 1;
        float friction = 1;
        float restitution = 0;
        void *ref = nullptr;
    };

    class Collider{
    public:
        enum Type{
            BOX,
            SPHERE,
        };

        glm::vec3 scale = {1, 1, 1};
        Type type;
    };

    class Physics {
    public:
        Physics();
        ~Physics();
        void init(glm::vec3 gravity = {0, 0, -9.81});
        void step(float deltaTime);
        void update(RigidBody &rb, Transform &t, Collider &collider, int index = -1);
        void add(RigidBody &rb, Transform &t, Collider &collider, int index = -1);
        void remove(RigidBody &rb);
        void rayCast(glm::vec3 from, glm::vec3 to, bool firstOnly, std::function<void(const glm::vec3 &pos, int index)> callback);

    private:
        class Impl;
        Ref<Impl> impl;
    };

}

#endif //TRIDOT_PHYSICS_H

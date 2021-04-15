//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_PHYSICS_H
#define TRIDOT_PHYSICS_H

#include "tridot/core/Ref.h"
#include "tridot/components/Transform.h"
#include <glm/glm.hpp>
#include <functional>

namespace tridot {

    class RigidBody{
    public:
        glm::vec3 velocity = {0, 0, 0};
        glm::vec3 angular = {0, 0, 0};
        float mass = 1;
        float friction = 1;
        float restitution = 0;
        float linearDamping = 0;
        float angularDamping = 0;
        void *physicsReference = nullptr;
        bool enablePhysics = true;

        glm::vec3 lastPosition = {0, 0, 0};
        glm::vec3 lastRotation = {0, 0, 0};
        glm::vec3 lastVelocity = {0, 0, 0};
        glm::vec3 lastAngular = {0, 0, 0};

        RigidBody(float mass = 1) : mass(mass){}
        RigidBody(const RigidBody& body);
    };

    class Collider{
    public:
        enum Type{
            BOX,
            SPHERE,
        };

        glm::vec3 scale;
        Type type;

        Collider(Type type = BOX, const glm::vec3 &scale = {1, 1, 1}) : type(type), scale(scale){}
    };

    class Physics {
    public:
        Physics();
        ~Physics();
        void init(glm::vec3 gravity = {0, 0, -9.81});
        void step(float deltaTime);
        void update(RigidBody &rigidBody, Transform &transform, Collider &collider, int index = -1);
        void add(RigidBody &rigidBody, Transform &transform, Collider &collider, int index = -1);
        void remove(RigidBody &rigidBody);
        void rayCast(glm::vec3 from, glm::vec3 to, bool firstOnly, std::function<void(const glm::vec3 &pos, int index)> callback);
        void contacts(RigidBody &rigidBody, std::function<void(const glm::vec3 &pos, int index)> callback);

    private:
        class Impl;
        Ref<Impl> impl;
    };

}

#endif //TRIDOT_PHYSICS_H

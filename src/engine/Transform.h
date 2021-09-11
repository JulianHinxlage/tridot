//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/config.h"
#include <glm/glm.hpp>

namespace tri {

    class Transform {
    public:
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 rotation;

        class Parent{
        public:
            EntityId id;
            glm::mat4 matrix;
        } parent;

        Transform(const glm::vec3 &position = {0, 0, 0}, const glm::vec3 &scale = {1, 1, 1}, const glm::vec3 &rotation = {0, 0, 0})
            : position(position), scale(scale), rotation(rotation) {
            parent.id = -1;
            parent.matrix = glm::mat4(1);
        }

        glm::mat4 getMatrix() const;
        glm::mat4 getLocalMatrix() const;
        void decompose(const glm::mat4 &matrix);

        static std::map<EntityId, std::vector<EntityId>> children;
        static bool hasChildren(EntityId id);
        static const std::vector<EntityId> &getChildren(EntityId id);
    };

}


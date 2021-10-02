//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/System.h"
#include <glm/glm.hpp>

namespace tri {

    class Transform {
    public:
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 rotation;

        EntityId parent;
        glm::mat4 matrix;

        Transform(const glm::vec3 &position = {0, 0, 0}, const glm::vec3 &scale = {1, 1, 1}, const glm::vec3 &rotation = {0, 0, 0})
            : position(position), scale(scale), rotation(rotation) {
            parent = -1;
            matrix = glm::mat4(1);
        }

        glm::mat4 calculateLocalMatrix() const;
        const glm::mat4 &getMatrix() const;
        void decompose(const glm::mat4 &matrix);
    };

    class HierarchySystem : public System {
    public:
        const std::vector<EntityId> &getChildren(EntityId id);
        bool isParentOf(EntityId id, EntityId parent);
        bool haveSameParent(EntityId id1, EntityId id2);
        void update() override;
        void startup() override;

    private:
        std::unordered_map<EntityId, std::vector<EntityId>> children;
        std::vector<EntityId> empty;
    };


}


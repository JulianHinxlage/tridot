//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/System.h"
#include "core/config.h"
#include <glm/glm.hpp>

namespace tri {

    class Transform {
    public:
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 rotation;
        EntityId parent = -1;

        Transform(const glm::vec3 &position = {0, 0, 0}, const glm::vec3 &scale = {1, 1, 1}, const glm::vec3 &rotation = {0, 0, 0})
            : position(position), scale(scale), rotation(rotation) {
            parent = -1;
            matrix = glm::mat4(1);
        }

        glm::mat4 calculateLocalMatrix() const;
        const glm::mat4& getMatrix() const;
        void decompose(const glm::mat4 &matrix);
        void setMatrix(const glm::mat4 &matrix);

        //only for main world (env->world)
        static const std::vector<EntityId>& getChilds(EntityId id);

    private:
        glm::mat4 matrix;
        friend class STransform;
    };

}


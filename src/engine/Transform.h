//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <glm/glm.hpp>

namespace tri {

    class Transform {
    public:
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 rotation;

        Transform(const glm::vec3 &position = {0, 0, 0}, const glm::vec3 &scale = {1, 1, 1}, const glm::vec3 &rotation = {0, 0, 0})
            : position(position), scale(scale), rotation(rotation) {}

        glm::mat4 calculateMatrix() const;
        void decompose(const glm::mat4 &matrix);
    };


}


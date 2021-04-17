//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_TRANSFORM_H
#define TRIDOT_TRANSFORM_H

#include <glm/glm.hpp>

namespace tridot {

    class Transform {
    public:
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 rotation;

        Transform(const glm::vec3 &position = {0, 0, 0}, const glm::vec3 &scale = {1, 1, 1}, const glm::vec3 &rotation = {0, 0, 0})
            : position(position), scale(scale), rotation(rotation) {}

        glm::mat4 getMatrix() const;
    };

}

#endif //TRIDOT_TRANSFORM_H

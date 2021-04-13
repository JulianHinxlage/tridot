//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Transform.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tridot {

    glm::mat4 Transform::getMatrix() {
        glm::mat4 transform(1);
        transform = glm::translate(transform, position);
        if(rotation != glm::vec3(0, 0, 0)){
            transform = glm::rotate(transform, rotation.z, {0, 0, 1});
            transform = glm::rotate(transform, rotation.y, {0, 1, 0});
            transform = glm::rotate(transform, rotation.x, {1, 0, 0});
        }
        transform = glm::scale(transform, scale);
        return transform;
    }

}
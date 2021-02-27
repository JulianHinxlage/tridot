//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace tridot {

    glm::mat4 Transform::getMatrix() {
        glm::mat4 transform(1);
        transform = glm::translate(transform, position);
        if(rotation != glm::vec3(0, 0, 0)){
            transform = transform * glm::eulerAngleXYZ(rotation.x, rotation.y, rotation.z);
        }
        transform = glm::scale(transform, scale);
        return transform;
    }

}
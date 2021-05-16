//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tridot {

    PerspectiveCamera::PerspectiveCamera() {
        forward = {0, 0, 1};
        up = {0, -1, 0};
        right = {1, 0, 0};
        fieldOfView = 60;
        near = 0.01;
        far = 1000;
        aspectRatio = 1.0;
        target = nullptr;
    }

    glm::mat4 PerspectiveCamera::getProjection() {
        forward = glm::normalize(forward);
        up = glm::normalize(up);
        right = glm::normalize(glm::cross(forward, up));
        return glm::perspective(glm::radians(fieldOfView), aspectRatio, near, far);
    }

    OrthographicCamera::OrthographicCamera() {
        scale = {1, 1};
        up = {0, -1};
        right = {1, 0};
        rotation = 0;
        aspectRatio = 0;
        target = nullptr;
    }

    glm::mat4 OrthographicCamera::getProjection() {
        return glm::ortho(-scale.x * aspectRatio, scale.x * aspectRatio, -scale.y, scale.y);
    }

}
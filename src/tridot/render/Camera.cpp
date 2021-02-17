//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tridot {

    PerspectiveCamera::PerspectiveCamera() {
        position = {0, 0, 0};
        forward = {0, 0, 1};
        up = {0, -1, 0};
        right = {1, 0, 0};
        fieldOfView = 60;
        near = 0.01;
        far = 1000;
        aspectRatio = 1.0;
    }

    glm::mat4 PerspectiveCamera::getProjection() {
        forward = glm::normalize(forward);
        up = glm::normalize(up);
        right = glm::normalize(glm::cross(forward, up));
        glm::mat4 projection = glm::lookAt(position, position + forward, up);
        projection = glm::perspective(glm::radians(fieldOfView), aspectRatio, near, far) * projection;
        return projection;
    }

    OrthographicCamera::OrthographicCamera() {
        position = {0, 0};
        scale = {1, 1};
        up = {0, -1};
        right = {1, 0};
        rotation = 0;
        aspectRatio = 0;
    }

    glm::mat4 OrthographicCamera::getProjection() {
        glm::mat4 projection =glm::ortho(-scale.x * aspectRatio, scale.x * aspectRatio, -scale.y, scale.y);
        projection = glm::rotate(projection, -rotation, glm::vec3(0, 0, 1));
        projection = glm::translate(projection, glm::vec3(-position, 0));
        return projection;
    }

}
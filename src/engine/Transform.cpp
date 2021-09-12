//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//


#include "Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>

namespace tri {

    glm::mat4 Transform::calculateMatrix() const {
        glm::mat4 transform(1);
        transform = glm::translate(transform, position);
        if (rotation.z != 0) {
            transform = glm::rotate(transform, rotation.z, { 0, 0, 1 });
        }
        if (rotation.y != 0) {
            transform = glm::rotate(transform, rotation.y, {0, 1, 0});
        }
        if (rotation.x != 0) {
            transform = glm::rotate(transform, rotation.x, {1, 0, 0});
        }
        transform = glm::scale(transform, scale);
        return transform;
    }

    void Transform::decompose(const glm::mat4 &matrix) {
        ImGuizmo::DecomposeMatrixToComponents((float*)&matrix, (float*)&position, (float*)&rotation, (float*)&scale);
        rotation = glm::radians(rotation);
    }

}

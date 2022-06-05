//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//


#include "Transform.h"
#include "core/core.h"
#include "entity/World.h"
#include "engine/Time.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtx/matrix_decompose.hpp>

namespace tri {

    glm::mat4 Transform::calculateLocalMatrix() const {
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

    const glm::mat4 &Transform::getMatrix() const {
        return matrix;
    }

    void Transform::decompose(const glm::mat4 &matrix) {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::quat orentiation;
        glm::decompose(matrix, scale, orentiation, position, skew, perspective);
        rotation = glm::eulerAngles(orentiation);
    }

    TRI_COMPONENT(Transform);
    TRI_PROPERTIES3(Transform, position, scale, rotation);
    TRI_PROPERTY_FLAGS(Transform, parent, PropertyDescriptor::HIDDEN);

    class STransform : public System {
    public:
        void tick() override {
            env->world->each<Transform>([](Transform& t) {
                t.matrix = t.calculateLocalMatrix();
            });
        }
    };
    TRI_SYSTEM(STransform);
}

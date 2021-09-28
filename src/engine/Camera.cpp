//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Camera.h"
#include "core/core.h"
#include "entity/Scene.h"
#include "Transform.h"
#include "render/Window.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tri {

    TRI_REGISTER_COMPONENT(Camera);
    TRI_REGISTER_MEMBER(Camera, forward);
    TRI_REGISTER_MEMBER(Camera, up);
    TRI_REGISTER_MEMBER(Camera, right);
    TRI_REGISTER_MEMBER(Camera, projection);
    TRI_REGISTER_MEMBER(Camera, target);
    TRI_REGISTER_MEMBER(Camera, output);
    TRI_REGISTER_MEMBER(Camera, pipeline);
    TRI_REGISTER_MEMBER(Camera, isPrimary);
    TRI_REGISTER_MEMBER(Camera, active);
    TRI_REGISTER_MEMBER(Camera, type);
    TRI_REGISTER_MEMBER(Camera, near);
    TRI_REGISTER_MEMBER(Camera, far);
    TRI_REGISTER_MEMBER_RANGE(Camera, fieldOfView, 30, 90);
    TRI_REGISTER_MEMBER(Camera, aspectRatio);

    TRI_REGISTER_TYPE(Camera::Type);
    TRI_REGISTER_CONSTANT(Camera::Type, PERSPECTIVE);
    TRI_REGISTER_CONSTANT(Camera::Type, ORTHOGRAPHIC);

    Camera::Camera(Type type, bool isPrimary) {
        forward = { 0, 0, -1 };
        up = { 0, 1, 0 };
        right = { 1, 0, 0 };
        projection = glm::mat4(1);
        target = nullptr;
        output = nullptr;
        pipeline = nullptr;
        this->isPrimary = isPrimary;
        this->type = type;
        active = true;
        near = 0.01;
        far = 1000;
        fieldOfView = 60;
        aspectRatio = 1.0f;
    }

    TRI_UPDATE_CALLBACK("Camera") {
        env->scene->view<Camera, Transform>().each([](Camera& camera, Transform &transform) {
            glm::mat4 t = transform.getMatrix();
            camera.forward = t * glm::vec4(0, 0, -1, 0);
            camera.right = t * glm::vec4(1, 0, 0, 0);
            camera.up = t * glm::vec4(0, 1, 0, 0);
            if (camera.type == Camera::PERSPECTIVE) {
                camera.projection = glm::perspective(glm::radians(camera.fieldOfView), camera.aspectRatio, camera.near, camera.far) * glm::inverse(t);
            }else if (camera.type == Camera::ORTHOGRAPHIC) {
                camera.projection = glm::ortho(-transform.scale.x * camera.aspectRatio, transform.scale.x * camera.aspectRatio, -transform.scale.y, transform.scale.y, camera.near, camera.far) * glm::inverse(t);
            }
        });
    }

}

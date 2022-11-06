//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Camera.h"
#include "core/core.h"
#include "entity/World.h"
#include "Transform.h"
#include "window/Viewport.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tri {

    TRI_COMPONENT(Camera);
    TRI_PROPERTIES8(Camera, forward, up, right, eyePosition, output, isPrimary, active, type);
    TRI_PROPERTIES4(Camera, near, far, fieldOfView, aspectRatio);
    TRI_PROPERTY_RANGE(Camera, fieldOfView, 30, 90);
    TRI_CLASS(Camera::Type);
    TRI_ENUM2(Camera::Type, PERSPECTIVE, ORTHOGRAPHIC);

    Camera::Camera(Type type, bool isPrimary) {
        forward = { 0, 0, -1 };
        up = { 0, 1, 0 };
        right = { 1, 0, 0 };
        eyePosition = { 0, 0, 0 };
        projection = glm::mat4(1);
        output = nullptr;
        this->isPrimary = isPrimary;
        this->type = type;
        active = true;
        near = 0.01;
        far = 1000;
        fieldOfView = 60;
        aspectRatio = 1.0f;
    }

    glm::vec3 Camera::getScreenRay(glm::vec2 screenPosition) {
        glm::vec2 center = env->viewport->position + env->viewport->size / 2;
        glm::vec2 rayAngles = (center - screenPosition) / (float)env->viewport->size.y * glm::radians(fieldOfView);

        glm::vec3 ray = forward;
        ray = glm::rotate(glm::mat4(1), rayAngles.x, up) * glm::vec4(ray, 1.0f);
        ray = glm::rotate(glm::mat4(1), rayAngles.y, right) * glm::vec4(ray, 1.0f);

        return glm::normalize(ray);
    }

    class SCamera : public System {
    public:
        void init() override {
            auto* job = env->jobManager->addJob("Renderer");
            job->addSystem<SCamera>();
        }

        void tick() override {
            env->world->each<const Transform, Camera>([](const Transform & transform, Camera & camera) {
                glm::mat4 t = transform.getMatrix();
                camera.eyePosition = transform.getWorldPosition();
                camera.forward = t * glm::vec4(0, 0, -1, 0);
                camera.right = t * glm::vec4(1, 0, 0, 0);
                camera.up = t * glm::vec4(0, 1, 0, 0);
                camera.view = glm::inverse(t);
                if (camera.type == Camera::PERSPECTIVE) {
                    camera.projection = glm::perspective(glm::radians(camera.fieldOfView), camera.aspectRatio, camera.near, camera.far);
                }
                else if (camera.type == Camera::ORTHOGRAPHIC) {
                    camera.projection = glm::ortho(-transform.scale.x * camera.aspectRatio, transform.scale.x * camera.aspectRatio, -transform.scale.y, transform.scale.y, camera.near, camera.far);
                }
                camera.transform = t;
                camera.viewProjection = camera.projection * camera.view;
            });
        }
    };
    TRI_SYSTEM(SCamera);

}

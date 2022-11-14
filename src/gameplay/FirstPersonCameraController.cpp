//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "FirstPersonCameraController.h"
#include "core/core.h"
#include "entity/World.h"
#include "engine/Transform.h"
#include "engine/Time.h"
#include "engine/Camera.h"
#include "window/Input.h"
#include "window/Viewport.h"
#include "physics/RigidBody.h"

namespace tri {

    TRI_COMPONENT_CATEGORY(FirstPersonCameraController, "Gameplay");
    TRI_PROPERTIES5(FirstPersonCameraController, active, followEntity, offset, mouseActive, mouseCanToggleActive);

    class FirstPersonCameraControllerSystem : public System {
    public:
        int mouseSetFrameNumber = -1;
        bool skipNext = true;

        void init() override {
            env->jobManager->addJob("Physics")->addSystem<FirstPersonCameraControllerSystem>();
        }

        glm::vec2 getMouseDelta(glm::vec2 startPos) {
            bool skip = false;
            if (mouseSetFrameNumber != env->time->frameCounter - 1) {
                skipNext = true;
                skip = true;
            }
            else {
                if (skipNext) {
                    skip = true;
                }
                skipNext = false;
            }
            mouseSetFrameNumber = env->time->frameCounter;

            glm::vec2 mousePosition = env->input->getMousePosition(false);
            glm::vec2 delta = mousePosition - startPos;
            if (skip) {
                delta = { 0, 0 };
            }
            env->input->setMousePosition(startPos, false);

            return delta;
        }

        void tick() override {
            env->world->each<FirstPersonCameraController, Camera, Transform>([&](EntityId id, FirstPersonCameraController& controller, Camera& camera, Transform& transform) {
                if (controller.active) {
                    if (controller.mouseCanToggleActive) {
                        if (env->input->pressed(Input::Key::KEY_TAB)) {
                            controller.mouseActive = !controller.mouseActive;
                        }
                    }

                    if (controller.mouseActive) {
                        glm::vec2 center = env->viewport->position + env->viewport->size / 2;
                        glm::vec2 delta = getMouseDelta(center);

                        //lock around
                        transform.rotation = controller.lastRotation;
                        transform.rotation.y -= delta.x * 0.001 / transform.scale.z;
                        transform.rotation.x -= delta.y * 0.001 / transform.scale.z;
                        transform.rotation.x = glm::radians(std::min(89.0f, std::max(-89.0f, glm::degrees(transform.rotation.x))));
                        transform.rotation.z = 0;
                        controller.lastRotation = transform.rotation;
                    }


                    glm::vec3 camForward = transform.calculateLocalMatrix() * glm::vec4(0, 0, -1, 0);
                    glm::vec3 camRight = transform.calculateLocalMatrix() * glm::vec4(1, 0, 0, 0);

                    glm::vec3 up = { 0, 1, 0 };
                    glm::vec3 forward = glm::cross(up, camRight);
                    glm::vec3 right = -glm::cross(up, forward);

                    glm::vec3 followPointOffset = controller.offset.x * right + controller.offset.y * up + controller.offset.z * forward;

                    if (controller.followEntity != -1) {
                        Transform* followTransform = env->world->getComponent<Transform>(controller.followEntity);
                        if (followTransform) {
                            transform.setWorldPosition(followTransform->getWorldPosition() + followPointOffset);
                        }
                    }
                }
            });

        }
    };
    TRI_SYSTEM(FirstPersonCameraControllerSystem);

}

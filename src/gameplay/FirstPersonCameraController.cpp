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
    TRI_PROPERTIES4(FirstPersonCameraController, active, followEntity, offset, mouseActive);

    class FirstPersonCameraControllerSystem : public System {
    public:
        int mouseSetFrameNumber = -1;
        bool skipNext = true;

        void tick() override {
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

            glm::vec2 center = env->viewport->position + env->viewport->size / 2;
            glm::vec2 mousePosition = env->input->getMousePosition(false);
            glm::vec2 delta = mousePosition - center;
            if (skip) {
                delta = { 0, 0 };
            }

            env->world->each<FirstPersonCameraController, Camera, Transform>([&](EntityId id, FirstPersonCameraController& controller, Camera& camera, Transform& transform) {
                if (controller.active) {
                    if (env->input->pressed(Input::Key::KEY_TAB)) {
                        controller.mouseActive = !controller.mouseActive;
                    }



                    if (controller.mouseActive) {
                        env->input->setMousePosition(center, false);
                        if (!skip) {
                            //lock around
                            transform.rotation = controller.lastRotation;
                            transform.rotation.y -= delta.x * 0.001 / transform.scale.z;
                            transform.rotation.x -= delta.y * 0.001 / transform.scale.z;
                            transform.rotation.x = glm::radians(std::min(89.0f, std::max(-89.0f, glm::degrees(transform.rotation.x))));
                            transform.rotation.z = 0;
                        }
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

    /*
    //FirstPersonCameraControlerSystem
	class FPCCSystem : public System {
	public:
        int mouseSetFrameNumber = -1;
        bool skipNext = true;

		void tick() override {
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

            glm::vec2 center = env->viewport->position + env->viewport->size / 2;
            glm::vec2 mousePosition = env->input->getMousePosition(false);
            glm::vec2 delta = mousePosition - center;
            if (skip) {
                delta = { 0, 0 };
            }

            env->world->each<FirstPersonCameraController, Camera, Transform>([&](EntityId id, FirstPersonCameraController &controller, Camera &camera, Transform &transform) {
                if (controller.active) {

                    if (controller.useWASD) {
                        //WASD
                        glm::vec3 move = { 0, 0, 0 };
                        if (env->input->down("W")) {
                            move.z -= 1;
                        }
                        if (env->input->down("A")) {
                            move.x -= 1;
                        }
                        if (env->input->down("S")) {
                            move.z += 1;
                        }
                        if (env->input->down("D")) {
                            move.x += 1;
                        }

                        if (move != glm::vec3(0, 0, 0 )) {
                            move = glm::normalize(move);
                        }

                        move *= controller.speed * env->time->frameTime;
                        move /= transform.scale;


                        glm::vec3 up = { 0, 1, 0 };
                        glm::vec3 forward = glm::cross(up, camera.right);
                        glm::vec3 right = -glm::cross(up, forward);

                        RigidBody *rb = env->world->getComponent<RigidBody>(id);
                        if (!rb) {
                            if (transform.parent != -1) {
                                rb = env->world->getComponent<RigidBody>(transform.parent);
                            }
                        }

                        if (!rb) {
                            glm::vec3 position = transform.getWorldPosition();
                            position += forward * -move.z;
                            position += right * move.x;
                            position += up * move.y;
                            if (transform.parent == -1) {
                                transform.setWorldPosition(position);
                            }
                            else {
                                Transform* parent = env->world->getComponent<Transform>(transform.parent);
                                if (parent) {
                                    parent->setWorldPosition(position - glm::vec3(parent->getMatrix() * glm::vec4(transform.position, 0)));
                                }
                                else {
                                    transform.setWorldPosition(position);
                                }
                            }
                        }
                        else {
                            rb->velocity.x *= 1.0f - env->time->deltaTime * controller.drag;
                            rb->velocity.z *= 1.0f - env->time->deltaTime * controller.drag;

                            rb->velocity += forward * -move.z * controller.drag;
                            rb->velocity += right * move.x * controller.drag;
                            rb->velocity += up * move.y * controller.drag;

                            rb->velocity.y = std::max(rb->velocity.y, -controller.maxFallSpeed);

                            if (skip) {
                                rb->velocity = { 0, 0, 0 };
                            }

                            Transform* parent = env->world->getComponent<Transform>(transform.parent);
                            if (parent) {
                                parent->rotation = {0, 0, 0};
                            }
                        }
                    }

                    if (env->input->pressed(Input::Key::KEY_TAB)) {
                        controller.mouseActive = !controller.mouseActive;
                    }

                    if (controller.mouseActive) {
                        env->input->setMousePosition(center, false);
                        if (!skip) {
                            //lock around
                            transform.rotation = controller.lastRotation;
                            transform.rotation.y -= delta.x * 0.001 / transform.scale.z;
                            transform.rotation.x -= delta.y * 0.001 / transform.scale.z;
                            transform.rotation.x = glm::radians(std::min(89.0f, std::max(-89.0f, glm::degrees(transform.rotation.x))));
                            transform.rotation.z = 0;
                        }
                        controller.lastRotation = transform.rotation;
                    }
                }
            });
            

		}
	};
	TRI_SYSTEM(FPCCSystem);
    */


}

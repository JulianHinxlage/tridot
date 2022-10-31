//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ThirdPersonCameraController.h"
#include "core/core.h"
#include "entity/World.h"
#include "engine/Transform.h"
#include "engine/Time.h"
#include "engine/Camera.h"
#include "window/Input.h"
#include "window/Viewport.h"
#include "physics/RigidBody.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tri {

	TRI_COMPONENT_CATEGORY(ThirdPersonCameraController, "Gameplay");
    TRI_PROPERTIES8(ThirdPersonCameraController, followEntity, followPoint, followSpeed, distance, playerSpeed, drag, maxFallSpeed, useWASD);
    TRI_PROPERTIES2(ThirdPersonCameraController, mouseActive, active);

    //ThirdPersonCameraControlerSystem
	class TPCCSystem : public System {
	public:
        int mouseSetFrameNumber = -1;
        bool skipNext = true;
        
        glm::vec3 lastPos;
        glm::vec3 lastPos2;

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

            env->world->each<ThirdPersonCameraController, const Camera, Transform>([&](EntityId id, ThirdPersonCameraController &controller, const Camera &camera, Transform &transform) {
                if (controller.active) {
                    if (controller.followEntity != -1) {
                        Transform* followTransform = env->world->getComponent<Transform>(controller.followEntity);
                        if (followTransform) {

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
                                if (move != glm::vec3(0, 0, 0)) {
                                    move = glm::normalize(move);
                                }

                                move *= controller.playerSpeed * env->time->frameTime;
                                move /= transform.scale;

                                glm::vec3 up = { 0, 1, 0 };
                                glm::vec3 forward = glm::cross(up, camera.right);
                                glm::vec3 right = -glm::cross(up, forward);

                                RigidBody* rb = env->world->getComponent<RigidBody>(controller.followEntity);
                                if (!rb) {
                                    glm::vec3 position = followTransform->getWorldPosition();
                                    position += forward * -move.z;
                                    position += right * move.x;
                                    position += up * move.y;

                                    if (followTransform->parent == -1) {
                                        followTransform->setWorldPosition(position);
                                    }
                                    else {
                                        Transform* parent = env->world->getComponent<Transform>(followTransform->parent);
                                        if (parent) {
                                            parent->setWorldPosition(position - glm::vec3(parent->getMatrix() * glm::vec4(followTransform->position, 0)));
                                        }
                                        else {
                                            followTransform->setWorldPosition(position);
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
                                }
                            }

                            if (env->input->pressed(Input::Key::KEY_TAB)) {
                                controller.mouseActive = !controller.mouseActive;
                            }

                            if (controller.mouseActive) {
                                env->input->setMousePosition(center, false);

                                //mouse wheel
                                float wheelDelta = env->input->getMouseWheelDelta();
                                controller.distance *= 1.0f - wheelDelta * 0.1f;

                                //look around
                                transform.rotation.y -= delta.x * 0.001 / transform.scale.z;
                                transform.rotation.x -= delta.y * 0.001 / transform.scale.z;
                                transform.rotation.x = glm::radians(std::min(89.0f, std::max(-89.0f, glm::degrees(transform.rotation.x))));
                                transform.rotation.z = 0;
                            }

                            //follow
                            glm::vec3 forward = transform.calculateLocalMatrix() * glm::vec4(0, 0, -1, 0);
                            glm::vec3 diff = followTransform->getWorldPosition() - controller.followPoint;
                            controller.followPoint = controller.followPoint + diff * glm::min(env->time->deltaTime * controller.followSpeed, 1.0f);
                            transform.setWorldPosition(controller.followPoint - forward * controller.distance);
                        }
                    }
                }
            });
		}
	};
	TRI_SYSTEM(TPCCSystem);

}

//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
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
    TRI_PROPERTIES8(ThirdPersonCameraController, active, followEntity, speed, distance, minDistance, maxDistance, offset, mouseActive);
    TRI_PROPERTIES1(ThirdPersonCameraController, mouseScrollActive);

    //ThirdPersonCameraControlerSystem
	class TPCCSystem : public System {
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

            env->world->each<ThirdPersonCameraController, const Camera, Transform>([&](EntityId id, ThirdPersonCameraController &controller, const Camera &camera, Transform &transform) {
                if (controller.active) {
                    if (env->input->pressed(Input::Key::KEY_TAB)) {
                        controller.mouseActive = !controller.mouseActive;
                    }

                    if (controller.mouseActive) {
                        env->input->setMousePosition(center, false);

                        if (controller.mouseScrollActive) {
                            //mouse wheel
                            float wheelDelta = env->input->getMouseWheelDelta();
                            controller.distance *= 1.0f - wheelDelta * 0.1f;
                            controller.distance = std::max(controller.minDistance, controller.distance);
                            controller.distance = std::min(controller.maxDistance, controller.distance);
                        }

                        //look around
                        transform.rotation.y -= delta.x * 0.001 / transform.scale.z;
                        transform.rotation.x -= delta.y * 0.001 / transform.scale.z;
                        transform.rotation.x = glm::radians(std::min(89.0f, std::max(-89.0f, glm::degrees(transform.rotation.x))));
                        transform.rotation.z = 0;
                    }

                    if (controller.followEntity != -1) {
                        Transform* followTransform = env->world->getComponent<Transform>(controller.followEntity);
                        if (followTransform) {
                            //follow
                            glm::vec3 camForward = transform.calculateLocalMatrix() * glm::vec4(0, 0, -1, 0);
                            glm::vec3 camRight = transform.calculateLocalMatrix() * glm::vec4(1, 0, 0, 0);

                            glm::vec3 up = { 0, 1, 0 };
                            glm::vec3 forward = glm::cross(up, camRight);
                            glm::vec3 right = -glm::cross(up, forward);

                            glm::vec3 followPointOffset = controller.offset.x * right + controller.offset.y * up + controller.offset.z * forward;

                            glm::vec3 diff = followTransform->getWorldPosition() - controller.followPoint;
                            controller.followPoint = controller.followPoint + diff * glm::min(env->time->deltaTime * controller.speed, 1.0f);
                            transform.setWorldPosition(controller.followPoint - camForward * controller.distance + followPointOffset);
                        }
                    }
                }
            });
		}
	};
	TRI_SYSTEM(TPCCSystem);

}

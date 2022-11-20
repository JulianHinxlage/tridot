//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "PlatformerPlayerController.h"
#include "core/core.h"
#include "entity/World.h"
#include "engine/Transform.h"
#include "physics/Physics.h"
#include "window/Input.h"
#include "engine/Time.h"
#include "engine/Camera.h"
#include "engine/EntityUtil.h"

namespace tri {

	TRI_COMPONENT_CATEGORY(PlatformerPlayerController, "Gameplay");
	TRI_PROPERTIES7(PlatformerPlayerController, speed, drag, maxFallSpeed, jumpHeight, jumpCount, jumpGracePeriode, active);

	class PlatformerPlayerControllerSystem : public System {
	public:
		void init() override {
			env->jobManager->addJob("Physics")->addSystem<PlatformerPlayerControllerSystem>();
		}

		void movement(EntityId id, Transform &transform, PlatformerPlayerController &controller) {
			//get camera right vector
			glm::vec3 camRight = { 1, 0, 0 };
			if (auto* cam = EntityUtil::getPrimaryCamera()) {
				camRight = cam->right;
			}

			//WASD input
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

			move *= controller.speed * env->time->frameTime;
			move /= transform.scale;

			glm::vec3 up = { 0, 1, 0 };
			glm::vec3 forward = glm::cross(up, camRight);
			glm::vec3 right = -glm::cross(up, forward);

			RigidBody* rb = env->world->getComponent<RigidBody>(id);
			if (!rb) {
				glm::vec3 position = transform.getWorldPosition();
				position += forward * -move.z;
				position += right * move.x;
				position += up * move.y;
				transform.setWorldPosition(position);
			}
			else {
				rb->velocity.x *= 1.0f - env->time->deltaTime * controller.drag;
				rb->velocity.z *= 1.0f - env->time->deltaTime * controller.drag;

				rb->velocity += forward * -move.z * controller.drag;
				rb->velocity += right * move.x * controller.drag;
				rb->velocity += up * move.y * controller.drag;

				rb->velocity.y = std::max(rb->velocity.y, -controller.maxFallSpeed);
			}
		}

		void tick() override {
			env->world->each<PlatformerPlayerController, Transform, RigidBody>([&](EntityId id, PlatformerPlayerController &c, Transform &t, RigidBody &rb) {
				if (c.active && EntityUtil::isEntityOwning(id)) {
					movement(id, t, c);

					glm::vec3 pos = t.getWorldPosition();
					float heightOffset = 0.5f;
					if (auto* collider = env->world->getComponent<Collider>(id)) {
						heightOffset = collider->scale.y * 0.5f;
					}

					//check if we are on the ground
					bool onGroundLast = c.onGround;
					c.onGround = false;
					//todo: use contact, not ray cast
					env->physics->rayCast(pos - glm::vec3(0, 0, 0), pos - glm::vec3(0, heightOffset + 0.05f, 0), false, [&](glm::vec3 pos, EntityId hitId) {
						if (hitId != id) {
							c.lastGroundContactTime = env->time->time;
							c.onGround = true;
						}
					});

					bool performJump = false;

					if (c.onGround) {
						c.hasJumpedCounter = 0;
						
						if (!onGroundLast) {
							//jump buffering
							if (c.lastJumpInputTime != -1 && c.lastJumpInputTime + 0.15f >= env->time->time) {
								performJump = true;
								c.lastJumpInputTime = -1;
							}
						}
					}

					//jump grace period
					if (c.lastGroundContactTime + c.jumpGracePeriode <= env->time->time && c.hasJumpedCounter == 0) {
						c.hasJumpedCounter = 1;
					}

					//jump input
					if (env->input->pressed(Input::KEY_SPACE)) {
						performJump = true;
						c.lastJumpInputTime = env->time->time;
					}

					//jump
					if (c.jumpCount > c.hasJumpedCounter) {
						if (performJump) {
							c.hasJumpedCounter++;
							rb.velocity.y = std::sqrt(c.jumpHeight * 2.0f * 9.81f);
						}
					}

					//jump height control
					if (env->input->released(Input::KEY_SPACE)) {
						if (rb.velocity.y > 0) {
							rb.velocity.y /= 2;
						}
					}

				}
			});
		}
	};
	TRI_SYSTEM(PlatformerPlayerControllerSystem);

}

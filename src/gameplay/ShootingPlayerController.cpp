//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Hitbox.h"
#include "core/core.h"
#include "engine/Transform.h"
#include "engine/Camera.h"
#include "engine/Prefab.h"
#include "engine/Time.h"
#include "window/Input.h"
#include "window/Viewport.h"
#include "physics/RigidBody.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tri {

	class ShootingPlayerController {
	public:
		float fireRate = 10;
		bool holdFire = true;
		bool directionHorizontal = true;
		float directionOffset = 0.5;
		bool useMouseAim = false;
		float projectileSpeed = 40;
		Ref<Prefab> projectile;
		float fireTimer = 0;
	};
	TRI_COMPONENT_CATEGORY(ShootingPlayerController, "Gameplay");
	TRI_PROPERTIES7(ShootingPlayerController, fireRate, holdFire, directionHorizontal, directionOffset, useMouseAim, projectileSpeed, projectile);

	class ShootingPlayerControllerSystem : public System {
	public:
		void tick() override {
			glm::vec3 camRight = { 1, 0, 0 };
			glm::vec3 camForward = { 0, 0, 1 };
			glm::vec3 camUp = { 0, 1, 0 };
			glm::vec3 camPosition = { 0, 0, 0 };
			float camFOV = 45;
			env->world->each<Camera>([&](Camera& c) {
				if (c.active && c.isPrimary) {
					camRight = c.right;
					camForward = c.forward;
					camUp = c.up;
					Transform t;
					t.decompose(c.transform);
					camPosition = t.position;
					camFOV = c.fieldOfView;
				}
			});

			env->world->each<ShootingPlayerController, Transform>([&](EntityId id, ShootingPlayerController&controller, Transform &transform) {
				if (controller.fireTimer > 0) {
					controller.fireTimer -= env->time->deltaTime;
				}
				if (env->input->down(Input::MOUSE_BUTTON_LEFT) && controller.holdFire 
					|| env->input->pressed(Input::MOUSE_BUTTON_LEFT) && !controller.holdFire) {
					if (controller.fireTimer <= 0) {
						controller.fireTimer = 1.0f / controller.fireRate;


						glm::vec3 position = transform.getWorldPosition();
						glm::vec3 forward = camForward;
						if (controller.useMouseAim) {
							if (controller.directionHorizontal) {
								glm::vec2 center = env->viewport->position + env->viewport->size / 2;
								glm::vec2 mousePosition = env->input->getMousePosition(false);
								glm::vec2 mouseOffset = (center - mousePosition) / (float)env->viewport->size.y * glm::radians(camFOV);

								forward = camForward;
								forward = glm::rotate(glm::mat4(1), mouseOffset.x, camUp) * glm::vec4(forward, 1.0f);
								forward = glm::rotate(glm::mat4(1), mouseOffset.y, camRight) * glm::vec4(forward, 1.0f);

								//mouse plane intersection
								 glm::vec3 intersect = camPosition - forward * ((camPosition.y - position.y) / forward.y);
								 forward = glm::normalize(intersect - position);
							}
						}
						else {
							if (controller.directionHorizontal) {
								glm::vec3 up = { 0, 1, 0 };
								forward = glm::cross(up, camRight);
							}
						}
						

						if (controller.projectile) {
							EntityId projectileId = controller.projectile->createEntity(env->world);
							Transform* projectileTransform = env->world->getComponentPending<Transform>(projectileId);
							if (!projectileTransform) {
								projectileTransform = &env->world->addComponent<Transform>(projectileId);
							}
							projectileTransform->position = position + forward * controller.directionOffset;

							if (Hitbox* projectileHitbox = env->world->getComponentPending<Hitbox>(projectileId)) {
								if (Hitbox* hitbox = env->world->getComponent<Hitbox>(id)) {
									projectileHitbox->team = hitbox->team;
								}
							}
							if (RigidBody* rb = env->world->getComponentPending<RigidBody>(projectileId)) {
								rb->velocity = forward * controller.projectileSpeed;
							}
						}

					}
				}
			});
		}
	};
	TRI_SYSTEM(ShootingPlayerControllerSystem);

}

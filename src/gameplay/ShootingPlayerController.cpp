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
#include "engine/EntityUtil.h"
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
			Camera* camera = EntityUtil::getPrimaryCamera();

			env->world->each<ShootingPlayerController, Transform>([&](EntityId id, ShootingPlayerController&controller, Transform &transform) {
				if (!EntityUtil::isEntityOwning(id)) {
					return;
				}
				if (controller.fireTimer > 0) {
					controller.fireTimer -= env->time->deltaTime;
				}
				if (!camera) {
					return;
				}
				if (env->input->down(Input::MOUSE_BUTTON_LEFT) && controller.holdFire 
					|| env->input->pressed(Input::MOUSE_BUTTON_LEFT) && !controller.holdFire) {
					if (controller.fireTimer <= 0) {
						controller.fireTimer = 1.0f / controller.fireRate;


						glm::vec3 position = transform.getWorldPosition();
						glm::vec3 forward = camera->forward;
						if (controller.useMouseAim) {
							if (controller.directionHorizontal) {
								glm::vec3 ray = camera->getScreenRay(env->input->getMousePosition(false));

								//mouse plane intersection
								glm::vec3 intersect = camera->eyePosition - ray * ((camera->eyePosition.y - position.y) / ray.y);
								forward = glm::normalize(intersect - position);
							}
						}
						else {
							if (controller.directionHorizontal) {
								glm::vec3 up = { 0, 1, 0 };
								forward = glm::cross(up, camera->right);
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

//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Hitbox.h"
#include "core/core.h"
#include "physics/Physics.h"
#include "engine/Time.h"
#include "engine/EntityUtil.h"

namespace tri {

	TRI_COMPONENT_CATEGORY(Hitbox, "Gameplay");
	TRI_PROPERTIES7(Hitbox, hitPoints, maxHitPoints, hitDamage, destroyOnImpact, team, lifeTime, onDestroy);

	class HitboxSystem : public System {
	public:
		void tick() override {
			env->world->each<Hitbox, RigidBody>([](EntityId id1, Hitbox &hitbox1, RigidBody& rb) {
				if (hitbox1.hitDamage > 0) {
					env->physics->contacts(rb, [&](glm::vec3 pos, EntityId id2) {
						if (hitbox1.lastHit != id2) {

							bool impactDestroy = false;
							if (Hitbox* hitbox2p = env->world->getComponent<Hitbox>(id2)) {
								Hitbox& hitbox2 = *hitbox2p;
								if (hitbox1.team != hitbox2.team) {
									if (hitbox2.maxHitPoints > 0) {
										hitbox2.hitPoints -= hitbox1.hitDamage;
										hitbox1.lastHit = id2;
										if (hitbox1.destroyOnImpact) {
											impactDestroy = true;
										}

									}
								}
							}
							else {
								if (hitbox1.destroyOnImpact) {
									if (RigidBody* rb2 = env->world->getComponent<RigidBody>(id2)) {
										if (!rb2->isTrigger) {
											impactDestroy = true;
										}
									}
								}
							}

							if (impactDestroy) {
								if (Transform* t1 = env->world->getComponent<Transform>(id1)) {
									glm::vec3 hitPos = pos;
									env->physics->rayCast(t1->getWorldPosition() - rb.velocity * env->time->deltaTime, pos, false,
										[&](glm::vec3 pos, EntityId id) {
											if (id == id2) {
												hitPos = pos;
											}
										});
									t1->setWorldPosition(hitPos);
									t1->updateMatrix();
								}
								hitbox1.onDestroy.invoke();
								EntityUtil::removeEntityWithChilds(id1);
							}

						}
					});
				}

				if (hitbox1.lifeTime != -1) {
					hitbox1.lifeTime -= env->time->deltaTime;
					if (hitbox1.lifeTime <= 0) {
						hitbox1.onDestroy.invoke();
						EntityUtil::removeEntityWithChilds(id1);
					}
				}

				if (hitbox1.maxHitPoints > 0) {
					if (hitbox1.hitPoints <= 0) {
						hitbox1.onDestroy.invoke();
						EntityUtil::removeEntityWithChilds(id1);
					}
				}
			});
		}
	};
	TRI_SYSTEM(HitboxSystem);

}

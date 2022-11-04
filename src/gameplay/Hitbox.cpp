//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Hitbox.h"
#include "core/core.h"
#include "physics/Physics.h"
#include "engine/Time.h"

namespace tri {

	TRI_COMPONENT_CATEGORY(Hitbox, "Gameplay");
	TRI_PROPERTIES7(Hitbox, hitPoints, maxHitPoints, hitDamage, destroyOnImpact, team, lifeTime, onDestroy);

	class HitboxSystem : public System {
	public:
		void tick() override {
			env->world->each<Hitbox, RigidBody>([](EntityId id1, Hitbox &hitbox1, RigidBody& rb) {
				env->physics->contacts(rb, [&](glm::vec3 pos, EntityId id2) {

					if (Hitbox *hitbox2 = env->world->getComponent<Hitbox>(id2)) {
						if (hitbox1.team != hitbox2->team) {
							if (hitbox1.maxHitPoints > 0) {
								if (hitbox2->hitDamage > 0 && hitbox2->lastHit != id1) {
									hitbox1.hitPoints -= hitbox2->hitDamage;
									hitbox1.lastHit = id2;
									hitbox2->lastHit = id1;
									if (hitbox2->destroyOnImpact) {
										hitbox2->onDestroy.invoke();
										env->world->removeEntity(id2);
									}
									if (hitbox1.hitPoints <= 0) {
										hitbox1.onDestroy.invoke();
										env->world->removeEntity(id1);
									}
								}
							}
						}
					}
					else {
						if (RigidBody* rb2 = env->world->getComponent<RigidBody>(id2)) {
							if (!rb2->isTrigger) {
								if (hitbox1.destroyOnImpact) {
									hitbox1.onDestroy.invoke();
									env->world->removeEntity(id1);
								}
							}
						}
					}
				});
				if (hitbox1.lifeTime != -1) {
					hitbox1.lifeTime -= env->time->deltaTime;
					if (hitbox1.lifeTime <= 0) {
						env->world->removeEntity(id1);
					}
				}
			});
		}
	};
	TRI_SYSTEM(HitboxSystem);

}

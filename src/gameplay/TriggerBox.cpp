//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "engine/EntityEvent.h"
#include "physics/Physics.h"

namespace tri {

	class Trigger {
	public:
	};
	TRI_COMPONENT_CATEGORY(Trigger, "Gameplay");

	class TriggerBox {
	public:
		EntityEvent enter;
		EntityEvent leaving;
		bool isTriggerd = false;
	};

	TRI_COMPONENT_CATEGORY(TriggerBox, "Gameplay");
	TRI_PROPERTIES2(TriggerBox, enter, leaving);

	class TriggerBoxSystem : public System {
	public:
		void tick() override {
			env->world->each<TriggerBox, RigidBody>([](TriggerBox &box, RigidBody &rb) {
				bool wasTriggerd = box.isTriggerd;
				box.isTriggerd = false;

				env->physics->contacts(rb, [&](glm::vec3 pos, EntityId id) {
					if (Trigger* trigger = env->world->getComponent<Trigger>(id)) {
						box.isTriggerd = true;
					}
				});

				if (!wasTriggerd && box.isTriggerd) {
					box.enter.invoke();
				}
				else if (wasTriggerd && !box.isTriggerd) {
					box.leaving.invoke();
				}
			});
		}
	};
	TRI_SYSTEM(TriggerBoxSystem);

}
//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "engine/EntityEvent.h"
#include "engine/Random.h"
#include "engine/Time.h"

namespace tri {

	class TimeTrigger {
	public:
		float delay = -1;
		float delayVariance = 0;
		float interval = -1;
		float intervalVariance = 0;
		EntityEvent trigger;
		float intervalTriggerTimer = -1;
		float delayTriggerTimer = -1;
		bool wasDelayTriggerd = false;
	};

	TRI_COMPONENT_CATEGORY(TimeTrigger, "Gameplay");
	TRI_PROPERTIES5(TimeTrigger, delay, delayVariance, interval, intervalVariance, trigger);

	class TimeTriggerSystem : public System {
	public:
		void tick() override {
			float time = env->time->inGameTime;
			env->world->each<TimeTrigger>([&](TimeTrigger& t) {
				if (t.delay >= 0 && !t.wasDelayTriggerd) {
					if (t.delayTriggerTimer < 0) {
						t.delayTriggerTimer = time + std::max(0.00f, t.delay + env->random->getFloat(-0.5f, 0.5f) * t.delayVariance);
					}
					if (t.delayTriggerTimer <= time) {
						t.trigger.invoke();
						t.delayTriggerTimer = -1;
						t.wasDelayTriggerd = true;
					}
				}
				if (t.interval >= 0) {
					if (t.intervalTriggerTimer < 0) {
						t.intervalTriggerTimer = time + std::max(0.00f, t.interval + env->random->getFloat(-0.5f, 0.5f) * t.intervalVariance);
					}
					if (t.intervalTriggerTimer <= time) {
						t.trigger.invoke();
						t.intervalTriggerTimer = time + std::max(0.00f, t.interval + env->random->getFloat(-0.5f, 0.5f) * t.intervalVariance);
					}
				}
			});
		}
	};
	TRI_SYSTEM(TimeTriggerSystem);

}
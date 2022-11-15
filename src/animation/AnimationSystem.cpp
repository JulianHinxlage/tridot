//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "AnimationComponent.h"
#include "core/core.h"
#include "engine/Time.h"
#include "engine/Transform.h"

namespace tri {

	class AnimationSystem : public System {
	public:
		float deltaTime = 0;

		void init() override {
			env->systemManager->addSystem<JobManager>();
			env->jobManager->addJob("Animation")->addSystem<AnimationSystem>();
		}

		void tick() override {
			deltaTime = env->time->deltaTime;
			env->world->getComponentStorage<Transform>()->lock();

			env->world->each<AnimationComponent>([&](EntityId id, AnimationComponent& anim) {
				if (anim.playing && anim.animation) {
					if (auto* t = env->world->getComponent<Transform>(id)) {
						if (!anim.lastPlaying) {
							glm::mat4 current = t->calculateLocalMatrix();
							t->position = { 0, 0, 0 };
							t->scale = { 1, 1, 1 };
							t->rotation = { 0, 0, 0 };
							anim.animation->apply(anim.time, id);
							glm::mat4 relative = t->calculateLocalMatrix();
							anim.startTransform = current * glm::inverse(relative);

							anim.maxTime = anim.animation->calculateMaxTime();
						}
						t->position = { 0, 0, 0 };
						t->scale = { 1, 1, 1 };
						t->rotation = { 0, 0, 0 };
					}

					float speed = anim.speed * (1 - (float)anim.reversed * 2);
					anim.time += speed * deltaTime;

					if (anim.maxTime == 0) {
						anim.maxTime = anim.animation->calculateMaxTime();
					}

					float overlap = 0;
					if (anim.reversed) {
						overlap = -anim.time;
					}
					else {
						overlap = anim.time - anim.maxTime;
					}
					if (overlap >= 0) {
						if (anim.pingPong) {
							if (anim.reversed && !anim.looping) {
								anim.playing = false;
							}
							anim.reversed = !anim.reversed;

							if (anim.reversed) {
								anim.time -= overlap * 2;
							}
							else {
								anim.time += overlap * 2;
							}
						}
						else {

							if (anim.reversed) {
								anim.time = anim.maxTime - overlap;
							}
							else {
								anim.time = overlap;
							}

							if (!anim.looping) {
								anim.playing = false;
							}
						}
					}

					if (anim.playing) {
						anim.animation->apply(anim.time, id);
					}
					else {
						if (anim.reversed != anim.pingPong) {
							anim.animation->apply(0, id);
							anim.time = 0;
						}
						else {
							anim.animation->apply(anim.maxTime, id);
							anim.time = anim.maxTime;
						}
					}

					if (auto* t = env->world->getComponent<Transform>(id)) {
						t->decompose(anim.startTransform * t->calculateLocalMatrix());
					}
				}
				anim.lastPlaying = anim.playing;
			});

			env->world->getComponentStorage<Transform>()->unlock();
		}
	};
	TRI_SYSTEM(AnimationSystem);

}

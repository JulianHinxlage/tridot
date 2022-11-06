//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ParticleEmitter.h"
#include "core/core.h"
#include "engine/Transform.h"
#include "engine/Time.h"
#include "engine/Random.h"
#include "engine/Camera.h"
#include "engine/RuntimeMode.h"
#include "engine/Serializer.h"
#include "engine/AssetManager.h"
#include "engine/MeshComponent.h"
#include "editor/Editor.h"
#include "core/Reflection.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace tri {

	TRI_CLASS_FLAGS(Particle, "Particle", "", ClassDescriptor::COMPONENT | ClassDescriptor::HIDDEN | ClassDescriptor::NO_SERIALIZE);
	TRI_PROPERTIES7(Particle, velocity, lifeTime, spawnTime, fadeInTime, fadeOutTime, color, faceCamera);

	TRI_ASSET(ParticleEffect);
	TRI_PROPERTIES8(ParticleEffect, particlesPerSecond, particlesPerTrigger, fadeInTime, fadeOutTime, velocity, velocityVariance, size, sizeVariance);
	TRI_PROPERTIES8(ParticleEffect, positionVariance, color, colorVariance, lifeTime, lifeTimeVariance, faceCamera, parrentParticles, inheritScale);
	TRI_PROPERTIES1(ParticleEffect, particle);

	TRI_COMPONENT(ParticleEmitter);
	TRI_PROPERTIES2(ParticleEmitter, active, effect);
	TRI_FUNCTION(ParticleEmitter, trigger);

	class ParticleSystem : public System {
	public:
		int maxParticels = 10000;

		void createParticle(const Transform &t, const ParticleEffect& e, EntityId source) {
			if (env->world->getComponentStorage<Particle>()->size() >= maxParticels) {
				return;
			}
			if (e.particle.size() > 0) {
				auto &prefab = e.particle[env->random->getInt(0, e.particle.size() - 1)];
				if (prefab) {
					EntityId id = prefab->createEntity();
					Transform* pt = &env->world->getOrAddComponentPending<Transform>(id);
					
					if (e.parrentParticles) {
						pt->parent = source;
						pt->position = e.positionVariance * (env->random->getVec3() * 2.0f - 1.0f);
					}
					else {
						Transform t2;
						pt->position = e.positionVariance * (env->random->getVec3() * 2.0f - 1.0f);
						pt->decompose(t.getMatrix() * pt->calculateLocalMatrix());
					}
					


					Particle& p = env->world->addComponent<Particle>(id);

					p.faceCamera = e.faceCamera;
					p.spawnTime = env->time->inGameTime;
					p.lifeTime = e.lifeTime + e.lifeTime * env->random->getFloat(-1, 1) * e.lifeTimeVariance;
					p.fadeInTime = e.fadeInTime;
					p.fadeOutTime = e.fadeOutTime;
					
					p.velocity = e.velocity + (env->random->getVec3() * 2.0f - 1.0f) * e.velocityVariance;
					if (!e.parrentParticles && e.inheritScale) {
						p.velocity = t.getMatrix() * glm::vec4(p.velocity, 0);
					}

					glm::vec3 size = e.size + (env->random->getVec3() * 2.0f - 1.0f) * e.sizeVariance;
					if (e.inheritScale) {
						pt->scale *= size;
					}
					else {
						pt->scale = size;
					}

					p.color = Color(e.color.vec() + (env->random->getVec4() * 2.0f - 1.0f) * e.colorVariance.vec());
					if (auto* mesh = env->world->getComponentPending<MeshComponent>(id)) {
						mesh->color = p.color;
						mesh->color.a = 0;
					}
				}
			}
		}

		void trigger(ParticleEffect& e, EntityId source = -1) {
			if (Transform* t = env->world->getComponent<Transform>(source)) {
				if (e.particlesPerTrigger >= 0) {
					for (int i = 0; i < e.particlesPerTrigger; i++) {
						createParticle(*t, e, source);
					}
				}
			}
		}

		void init() override {
			env->console->addCVar("maxParticles", &maxParticels);
			env->jobManager->addJob("ParticleSystem")->addSystem<ParticleSystem>();
		}

		virtual void startup() override {
			env->runtimeMode->setActiveSystem(RuntimeMode::PAUSED, "ParticleSystem", true);

			if (env->editor) {
				env->editor->fileAssosiations[".effect"] = Reflection::getClassId<ParticleEffect>();
			}
		}

		virtual void tick() override {
			env->world->each<const ParticleEmitter, const Transform>([&](EntityId id, const ParticleEmitter &e, const Transform &t) {
				if (e.active) {
					if (e.effect && e.effect->particlesPerSecond > 0) {
						int count = env->time->deltaTicks(1.0f / e.effect->particlesPerSecond);
						for (int i = 0; i < count; i++) {
							createParticle(t, *e.effect, id);
						}
					}
				}
			});

			const Transform *cameraTransform = nullptr;
			env->world->each<const Camera, const Transform>([&](const Camera& c, const Transform& t) {
				if (c.isPrimary && c.active) {
					cameraTransform = &t;
				}
			});

			env->world->each<const Particle, const Transform>([&](EntityId id, const Particle& p, Transform& t) {
				t.position += p.velocity * env->time->deltaTime;

				if (p.faceCamera && cameraTransform) {
					//todo: use a faster formular
					Transform t2;
					glm::mat mat = glm::lookAt(t.getWorldPosition(), cameraTransform->getWorldPosition(), {0, 0, 1});
					t2.decompose(glm::rotate(glm::inverse(mat), glm::radians(-90.0f), {1, 0, 0}));
					t.rotation = t2.rotation;
				}

				if (auto* mesh = env->world->getComponent<MeshComponent>(id)) {
					float factor = 1;
					float time = env->time->inGameTime - p.spawnTime;
					factor *= glm::clamp(time / p.fadeInTime, 0.0f, 1.0f);
					factor *= glm::clamp((p.lifeTime - time) / p.fadeOutTime, 0.0f, 1.0f);
					mesh->color.a = p.color.a * factor;
				}

				if (env->time->inGameTime >= p.spawnTime + p.lifeTime) {
					env->world->removeEntity(id);
				}
			});
		}
	};
	TRI_SYSTEM(ParticleSystem);

	void ParticleEmitter::trigger() {
		if (effect) {
			env->systemManager->getSystem<ParticleSystem>()->trigger(*effect, env->world->getIdByComponent(this));
		}
	}

	bool ParticleEffect::save(const std::string& file) {
		env->assetManager->setOptions(file, AssetManager::Options::NO_RELOAD_ONCE);
		SerialData data;
		env->serializer->saveToFile(data, file);
		env->serializer->serializeClass(this, data);
		return true;
	}

	bool ParticleEffect::load(const std::string& file) {
		SerialData data;
		if (env->serializer->loadFromFile(data, file)) {
			env->serializer->deserializeClass(this, data);
			return true;
		}
		return false;
	}

}

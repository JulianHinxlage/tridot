//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "engine/Prefab.h"
#include "engine/Color.h"
#include <glm/glm.hpp>

namespace tri {

	class Particle {
	public:
		glm::vec3 velocity = {0, 0, 0};
		float lifeTime = 0;
		float spawnTime = 0;
		float fadeInTime = 0;
		float fadeOutTime = 0;
		Color color;
		bool faceCamera = false;
	};

	class ParticleEffect : public Asset {
	public:
		float particlesPerSecond = 10;
		int particlesPerTrigger = 10;

		float fadeInTime = 0.1;
		float fadeOutTime = 0.1;

		glm::vec3 velocity = {0, 0, 0};
		glm::vec3 velocityVariance = {1, 1, 1};

		glm::vec3 size = { 1, 1, 1 };
		glm::vec3 sizeVariance = { 0.1, 0.1, 0.1 };

		glm::vec3 positionVariance = { 0, 0, 0 };


		Color color = color::white;
		Color colorVariance = color::black;

		float lifeTime = 1;
		float lifeTimeVariance = 0.1;

		bool faceCamera = false;
		bool parrentParticles = false;

		std::vector<Ref<Prefab>> particle;

		bool save(const std::string &file) override;
		bool load(const std::string& file) override;
	};

	class ParticleEmitter {
	public:
		bool active = true;
		Ref< ParticleEffect> effect;

		void trigger();
	};

}

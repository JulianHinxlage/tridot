//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include <random>
#include <glm/glm.hpp>

namespace tri {

	class Random : public System {
	public:
		virtual void startup() override;
		void pushSeed(uint64_t seed);
		void popSeed();

		float getFloat();
		float getFloat(float min, float max);
		int getInt();
		int getInt(int min, int max);
		uint64_t getUint64();
		float getNormal();
		glm::vec2 getVec2();
		glm::vec3 getVec3();
		glm::vec4 getVec4();
		glm::vec2 getVec2Normal();
		glm::vec3 getVec3Normal();
		glm::vec4 getVec4Normal();

	private:
		std::mt19937_64 rng;
		std::vector<std::mt19937_64> rngStack;
	};

}


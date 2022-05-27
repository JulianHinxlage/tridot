//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Random.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(Random, env->random);

	void Random::startup() {
		pushSeed(std::random_device()());
	}

	void Random::pushSeed(uint64_t seed) {
		if (rngStack.size() > 0) {
			rngStack.back() = rng;
		}
		rngStack.emplace_back(std::mt19937_64());
		rngStack.back().seed(seed);
		rng = rngStack.back();
	}

	void Random::popSeed() {
		if (rngStack.size() > 1) {
			rngStack.pop_back();
			rng = rngStack.back();
		}
	}

	float Random::getFloat() {
		return std::uniform_real_distribution<float>()(rng);
	}

	float Random::getFloat(float min, float max) {
		return std::uniform_real_distribution<float>(min, max)(rng);
	}

	int Random::getInt() {
		return std::uniform_int_distribution<int>()(rng);
	}

	int Random::getInt(int min, int max) {
		return std::uniform_int_distribution<int>(min, max)(rng);
	}

	uint64_t Random::getUint64() {
		return std::uniform_int_distribution<uint64_t>()(rng);
	}

	float Random::getNormal() {
		return std::normal_distribution<float>()(rng);
	}

	glm::vec2 Random::getVec2() {
		return { getFloat(), getFloat() };
	}

	glm::vec3 Random::getVec3() {
		return { getFloat(), getFloat(), getFloat() };
	}

	glm::vec4 Random::getVec4() {
		return { getFloat(), getFloat(), getFloat(), getFloat() };
	}

	glm::vec2 Random::getVec2Normal() {
		return { getNormal(), getNormal() };
	}

	glm::vec3 Random::getVec3Normal() {
		return { getNormal(), getNormal(), getNormal() };
	}

	glm::vec4 Random::getVec4Normal() {
		return { getNormal(), getNormal(), getNormal(), getNormal() };
	}

}

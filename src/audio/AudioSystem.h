//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/System.h"
#include "Audio.h"
#include <glm/glm/glm.hpp>

namespace tri {

	class AudioSystem : public System {
	public:
		void init() override;
		void startup() override;
		void tick() override;
		void shutdown() override;

		uint32_t play(const Ref<Audio>& audio, const glm::vec3& pos = {0, 0, 0});
		void stop(uint32_t id);
	private:
		std::vector<uint32_t> sources;
		std::vector<Ref<Audio>> loadingQueue;
	};

}

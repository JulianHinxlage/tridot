//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include <glm/glm.hpp>

namespace tri {

	class FrameBuffer;
	class Texture;
	class Viewport : public System {
	public:
		glm::ivec2 size = { 0, 0 };
		glm::ivec2 position = {0, 0};
		bool displayInWindow = true;
		Ref<FrameBuffer> frameBuffer;
		Ref<Texture> idMap;

		void init() override;
		void shutdown() override;
	};

}
//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include <glm/glm.hpp>

namespace tri {

	class FrameBuffer;
	class Viewport : public System {
	public:
		glm::ivec2 size = {0, 0};
		bool displayInWindow = true;
		Ref<FrameBuffer> frameBuffer;

		void init() override;
		void shutdown() override;
	};

}
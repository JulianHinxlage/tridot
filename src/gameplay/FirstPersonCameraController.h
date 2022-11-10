//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "entity/World.h"
#include <glm/glm.hpp>

namespace tri {

	class FirstPersonCameraController {
	public:
		bool active = true;
		EntityId followEntity = -1;
		glm::vec3 offset = {0, 0, 0};
		bool mouseActive = true;
		bool mouseCanToggleActive = true;
		glm::vec3 lastRotation = {0, 0, 0};
	};

}

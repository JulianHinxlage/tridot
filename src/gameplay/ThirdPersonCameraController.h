//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "entity/World.h"
#include <glm/glm.hpp>

namespace tri {

	class ThirdPersonCameraController {
	public:
		bool active = true;
		EntityId followEntity = -1;
		float speed = 10;
		float distance = 10;
		float minDistance = 1;
		float maxDistance = 100;
		glm::vec3 offset = {0, 0, 0};
		bool mouseActive = true;
		bool mouseScrollActive = true;
		glm::vec3 followPoint;
	};

}

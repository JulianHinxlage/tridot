//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "entity/World.h"
#include <glm/glm.hpp>

namespace tri {

	class ThirdPersonCameraController {
	public:
		EntityId followEntity = -1;
		glm::vec3 followPoint;
		float followSpeed = 10;
		float distance = 10;
		float playerSpeed = 10;
		float drag = 5;
		float maxFallSpeed = 40;
		bool useWASD = true;
		bool active = true;
		glm::vec3 followPointVelocity;
	};

}

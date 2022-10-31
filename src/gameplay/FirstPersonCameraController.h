//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include <glm/glm.hpp>

namespace tri {

	class FirstPersonCameraController {
	public:
		float speed = 5;
		float drag = 5;
		float maxFallSpeed = 40;
		glm::vec3 lastRotation;
		bool useWASD = true;
	};

}

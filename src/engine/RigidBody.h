//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include <glm/glm.hpp>

namespace tri {

	class RigidBody {
	public:
		enum Type {
			STATIC,
			DYNAMIC,
		};
		Type type = STATIC;

		glm::vec3 velocity = { 0, 0, 0 };
		glm::vec3 angular = { 0, 0, 0 };
		float mass = 1;
		float friction = 1;
		float restitution = 0;
		float linearDamping = 0;
		float angularDamping = 0;
		bool enablePhysics = true;

		glm::vec3 lastPosition = { 0, 0, 0 };
		glm::vec3 lastRotation = { 0, 0, 0 };
		glm::vec3 lastVelocity = { 0, 0, 0 };
		glm::vec3 lastAngular = { 0, 0, 0 };

		void* reference = nullptr;
	};

	class Collider {
	public:
		enum Type {
			BOX,
			SPHERE,
			MESH,
			CONCAVE_MESH,
		};
		Type type;
		glm::vec3 size;

		Collider(Type type = BOX, glm::vec3 size = glm::vec3(1, 1, 1));
	};

}
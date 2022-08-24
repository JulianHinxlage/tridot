//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include <glm/glm.hpp>

namespace tri {

	class RigidBody {
	public:
		enum Type {
			STATIC,
			DYNAMIC,
			KINEMATIC,
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
		void* reference = nullptr;

		glm::vec3 lastPosition = { 0, 0, 0 };
		glm::vec3 lastRotation = { 0, 0, 0 };
		glm::vec3 lastVelocity = { 0, 0, 0 };
		glm::vec3 lastAngular = { 0, 0, 0 };
	};

}

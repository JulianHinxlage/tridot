//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include <glm/glm.hpp>

namespace tri {

	class Collider {
	public:
		enum Type {
			BOX,
			SPHERE,
			CAPSULE,
			MESH,
			CONCAVE_MESH,
		};

		Type type = BOX;
		glm::vec3 scale = {1, 1, 1};
		glm::vec3 offset = {0, 0, 0};
	};

}

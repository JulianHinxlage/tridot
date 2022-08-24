//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Collider.h"

namespace tri {

	TRI_ENUM3(Collider::Type, BOX, SPHERE, CAPSULE, MESH, CONCAVE_MESH);

	TRI_COMPONENT_CATEGORY(Collider, "Physics");
	TRI_PROPERTIES3(Collider, type, scale, offset);

}
//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "RigidBody.h"

namespace tri {

	TRI_ENUM3(RigidBody::Type, STATIC, DYNAMIC, KINEMATIC);

	TRI_COMPONENT_CATEGORY(RigidBody, "Physics");
	TRI_PROPERTIES8(RigidBody, type, velocity, angular, mass, friction, restitution, linearDamping, angularDamping, enablePhysics);
	TRI_PROPERTIES2(RigidBody, enablePhysics, isTrigger);

}
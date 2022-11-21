//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "RigidBody.h"

namespace tri {

	TRI_ENUM3(RigidBody::Type, STATIC, DYNAMIC, KINEMATIC);

	TRI_COMPONENT_CATEGORY(RigidBody, "Physics");
	
	TRI_PROPERTIES1(RigidBody, type);
	TRI_PROPERTY_FLAGS(RigidBody, velocity, PropertyDescriptor::REPLICATE);
	TRI_PROPERTY_FLAGS(RigidBody, angular, PropertyDescriptor::REPLICATE);
	TRI_PROPERTIES6(RigidBody, mass, friction, restitution, linearDamping, angularDamping, enablePhysics);
	TRI_PROPERTIES3(RigidBody, enablePhysics, isTrigger, enableGravity);

}
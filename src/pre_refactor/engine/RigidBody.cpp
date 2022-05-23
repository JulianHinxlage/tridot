//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "RigidBody.h"
#include "core/core.h"

namespace tri {

	TRI_REGISTER_TYPE(RigidBody::Type);
	TRI_REGISTER_CONSTANT(RigidBody::Type, STATIC);
	TRI_REGISTER_CONSTANT(RigidBody::Type, DYNAMIC);
	TRI_REGISTER_CONSTANT(RigidBody::Type, KINEMATIC);

	TRI_REGISTER_COMPONENT(RigidBody);
	TRI_REGISTER_MEMBER(RigidBody, type);
	TRI_REGISTER_MEMBER(RigidBody, velocity);
	TRI_REGISTER_MEMBER(RigidBody, angular);
	TRI_REGISTER_MEMBER(RigidBody, mass);
	TRI_REGISTER_MEMBER(RigidBody, friction);
	TRI_REGISTER_MEMBER(RigidBody, restitution);
	TRI_REGISTER_MEMBER(RigidBody, linearDamping);
	TRI_REGISTER_MEMBER(RigidBody, angularDamping);
	TRI_REGISTER_MEMBER(RigidBody, enablePhysics);


	TRI_REGISTER_TYPE(Collider::Type);
	TRI_REGISTER_CONSTANT(Collider::Type, BOX);
	TRI_REGISTER_CONSTANT(Collider::Type, SPHERE);
	TRI_REGISTER_CONSTANT(Collider::Type, MESH);
	TRI_REGISTER_CONSTANT(Collider::Type, CONCAVE_MESH);

	TRI_REGISTER_COMPONENT(Collider);
	TRI_REGISTER_MEMBER(Collider, type);
	TRI_REGISTER_MEMBER(Collider, size);

	Collider::Collider(Type type, glm::vec3 size) : type(type), size(size) {}

}
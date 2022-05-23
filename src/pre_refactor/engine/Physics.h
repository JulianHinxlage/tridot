//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "RigidBody.h"
#include "Transform.h"
#include "core/System.h"
#include "core/util/Ref.h"

namespace tri {

	class Physics : public System {
	public:
		glm::vec3 gravity = { 0, -9.81, 0 };

		virtual void update() override;
		virtual void startup() override;

		void rayCast(glm::vec3 from, glm::vec3 to, bool firstOnly, std::function<void(const glm::vec3& pos, EntityId id)> callback);
		void contacts(RigidBody& rigidBody, std::function<void(const glm::vec3& pos, EntityId id)> callback);

	private:
		class Impl;
		Ref<Impl> impl;

		void addRigidBody(EntityId& id, RigidBody& rigidBody, Collider& collider, Transform& transform);
		void removeRigidBody(EntityId& id, RigidBody& rigidBody, Collider& collider, Transform& transform);
	};

}
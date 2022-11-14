//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "Collider.h"
#include "RigidBody.h"
#include "engine/Transform.h"
#include "engine/RuntimeMode.h"

namespace tri {

	class Physics : public System {
	public:
		void init() override;
		void startup() override;
		void tick() override;
		void shutdown() override;

		void rayCast(glm::vec3 from, glm::vec3 to, bool firstOnly, std::function<void(const glm::vec3& pos, EntityId id)> callback);
		void contacts(RigidBody& rigidBody, std::function<void(const glm::vec3& pos, EntityId id)> callback);

	private:
		class Impl;
		std::shared_ptr<Impl> impl;

		int entityRemoveListener;
		int entityDeactivatedListener;
		int modeChangeListener;
		int endMapListener;
		RuntimeMode::Mode lastFrameRuntimeMode = RuntimeMode::LOADING;

		void addRigidBody(EntityId& id, RigidBody& rigidBody, Transform& transform);
		void removeRigidBody(EntityId& id, RigidBody& rigidBody, Transform& transform);
	};

}

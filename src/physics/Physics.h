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

		void addRigidBody(EntityId& id, RigidBody& rigidBody, Transform& transform);
		void removeRigidBody(EntityId& id, RigidBody& rigidBody, Transform& transform);

	private:
		class Impl;
		std::shared_ptr<Impl> impl;

		int entityRemoveListener;
		int modeChangeListener;
		int endMapListener;
		RuntimeMode::Mode lastFrameRuntimeMode = RuntimeMode::LOADING;
	};

}

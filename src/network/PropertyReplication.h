//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/System.h"
#include "entity/ComponentStorage.h"
#include "Connection.h"

namespace tri {

	class PropertyReplication : public System {
	public:
		float networkReplicationRate = 60.0f;

		void init() override;
		void startup() override;
		void tick() override;

		void replicateToConnection(Connection* conn);
		void updateShadowState();

	private:
		std::vector<std::vector<std::shared_ptr<ComponentStorage>>> storages;
		std::mutex mutex;
	};

}

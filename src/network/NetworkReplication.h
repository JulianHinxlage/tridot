//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/util/Guid.h"
#include "core/System.h"
#include "entity/World.h"
#include "NetworkManager.h"
#include "Packet.h"

namespace tri {

	class NetworkReplication : public System {
	public:
		void init() override;
		void startup() override;
		void tick() override;
		void shutdown() override;

	private:
		std::set<Guid> addedRuntimeEntities;
		std::set<Guid> removedRuntimeEntities;
		std::set<Guid> mapEntities;
		std::set<Guid> removedMapEntities;
	};

}

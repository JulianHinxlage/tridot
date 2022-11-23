//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/util/Guid.h"
#include "core/System.h"
#include "entity/World.h"
#include "NetworkManager.h"

namespace tri {

	class NetworkReplication : public System {
	public:
		bool enableClientSideEntitySpawning;
		bool enableClientSideEntityDepawning;

		void init() override;
		void startup() override;
		void tick() override;
		void shutdown() override;

		void setOwning(Guid guid, Connection *conn);
		bool isOwning(Guid guid);
		Connection* getOwningConnection(Guid guid);

		void addEntity(EntityId id, Guid guid, Connection* conn = nullptr);
		void updateEntity(EntityId id, Guid guid, Connection* conn = nullptr);
		void removeEntity(Guid guid, Connection* conn = nullptr);

	private:
		std::set<Guid> addedRuntimeEntities;
		std::set<Guid> removedRuntimeEntities;
		std::set<Guid> mapEntities;
		std::set<Guid> removedMapEntities;

		std::set<Guid> owning;
		std::map<Guid, Connection*> owningConnections;
		std::set<Guid> addedNetworkEntities;
		std::set<Guid> removedNetworkEntities;


		std::map<EntityId, EntityId> idMap;
	};

}

//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/util/Guid.h"
#include "core/System.h"
#include "entity/World.h"
#include "NetworkSystem.h"

namespace tri {

	class NetworkReplication : public System {
	public:
		bool active;
		bool hasAuthority;

		NetworkReplication();

		void init() override;
		void startup() override;
		void tick() override;
		void shutdown() override;

		void onData(Guid guid, NetOpcode opcode, const std::string &data);

	private:
		std::set<Guid> addedRuntimeEntities;
		std::set<Guid> removedRuntimeEntities;

		int addListener;
		int componentAddListener;
		int removeListener;
		int beginListener;

		class Operation {
		public:
			Guid guid;
			NetOpcode opcode;
			std::string data;
		};
		std::vector<Operation> operations;
		std::mutex operationsMutex;

		void updateEntity(EntityId id);
		void addEntity(EntityId id);
		void removeEntity(EntityId id);
	};

}

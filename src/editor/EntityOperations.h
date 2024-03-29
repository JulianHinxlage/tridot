//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include "engine/Prefab.h"
#include "entity/DynamicObjectBuffer.h"

namespace tri {

	class EntityOperations : public System {
	public:
		void init() override;

		EntityId duplicateEntity(EntityId id, World *world = nullptr);
		void duplicateSelection();

		void copyEntity(EntityId id, World* world = nullptr);
		EntityId pastEntity(World* world = nullptr);
		void removeEntity(EntityId id, World* world = nullptr);

		void copyComponent(EntityId id, int classId, World* world = nullptr);
		void pastComponent(EntityId id, World* world = nullptr);
		void removeComponent(EntityId id, int classId, World* world = nullptr);
		void parentEntity(EntityId id, EntityId parent, World* world = nullptr);

		bool hasCopiedEntity();
		int getCopiedComponentClassId();

		void copyFunction(EntityId entityId, int classId, int functionIndex);
		void pastFunction(EntityId &entityId, int &classId, int &functionIndex);
		bool hasCopiedFunction();

		void copyEvent(EntityId entityId, int classId, int propertyIndex);
		void pastEvent(EntityId& entityId, int& classId, int& propertyIndex);
		bool hasCopiedEvent();

	private:
		Prefab entityBuffer;
		bool hasEntityInBuffer;
		DynamicObjectBuffer componentBuffer;

		EntityId functionEntityId = -1;
		int functionClassId = -1;
		int functionIndex = -1;

		EntityId eventEntityId = -1;
		int eventClassId = -1;
		int eventPropertyIndex = -1;
	};

}

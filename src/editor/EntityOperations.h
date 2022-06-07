//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include "entity/Prefab.h"

namespace tri {

	class EntityOperations : public System {
	public:
		void init() override;

		EntityId duplicateEntity(EntityId id, World *world = nullptr);
		void copyEntity(EntityId id, World* world = nullptr);
		EntityId pastEntity(World* world = nullptr);
		void removeEntity(EntityId id, World* world = nullptr);

		void copyComponent(EntityId id, int classId, World* world = nullptr);
		void pastComponent(EntityId id, World* world = nullptr);
		void removeComponent(EntityId id, int classId, World* world = nullptr);
		void parentEntity(EntityId id, EntityId parent, World* world = nullptr);

		bool hasCopiedEntity();
		int getCopiedComponentClassId();

	private:
		Prefab entityBuffer;
		bool hasEntityInBuffer;
		DynamicObjectBuffer componentBuffer;
	};

}

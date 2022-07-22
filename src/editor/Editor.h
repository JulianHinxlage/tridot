//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/System.h"
#include "core/Reflection.h"
#include "entity/World.h"
#include "ClassUI.h"
#include "UndoSystem.h"
#include "SelectionContext.h"
#include "EntityOperations.h"

namespace tri {

	class Editor : public System {
	public:
		ClassUI *classUI;
		UndoSystem* undo;
		SelectionContext* selectionContext;
		EntityOperations* entityOperations;

		EntityId dragEntityId = -1;

		void init() override;
		void startup() override;
		void tick() override;
		void shutdown() override;

		int setPersistentEntity(EntityId id, int handle = -1);
	private:
		int autoSaveListener;
		int crashSaveListener;
		int playBufferListener;
		World* playBuffer;
		std::vector<std::pair<int, EntityId>> playModePersistentEntities;
		float lastUnhandledExceptionTime = -1;
	};

}

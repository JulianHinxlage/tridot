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
		std::unordered_map<std::string, int> fileAssosiations;

		EntityId dragEntityId = -1;
		bool propertiesNoContext = false;
		bool propertiesNoScroll = false;

		void init() override;
		void startup() override;
		void tick() override;
		void shutdown() override;

		int setPersistentEntity(EntityId id, int handle = -1);
		std::string openFileDialog(const char* pattern, bool allowNewFile = false);
	private:
		int autoSaveListener;
		int crashSaveListener;
		int playBufferListener;
		World* playBuffer;
		std::vector<std::pair<int, EntityId>> playModePersistentEntities;
		float lastUnhandledExceptionTime = -1;

		void saveMap(const std::string& file);
	};

}

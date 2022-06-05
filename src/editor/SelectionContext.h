//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/core.h"

namespace tri {

	class SelectionContext : public System {
	public:
		void init() override;
		void select(EntityId id, bool unselectOther = true);
		void unselect(EntityId id);
		void unselectAll();

		bool isSelected(EntityId id);

		bool isAnySelected();
		bool isSingleSelection();
		bool isMultiSelection();

		const std::vector<EntityId>& getSelected();

	private:
		std::vector<EntityId> selectedEntities;
	};

}
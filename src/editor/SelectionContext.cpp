//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "SelectionContext.h"
#include "Editor.h"

namespace tri {

	TRI_SYSTEM(SelectionContext);

	void SelectionContext::init() {
		env->editor->selectionContext = this;
	}

	void SelectionContext::select(EntityId id, bool unselectOther) {
		if (unselectOther) {
			unselectAll();
		}
		for (int i = 0; i < selectedEntities.size(); i++) {
			if (selectedEntities[i] == id) {
				return;
			}
		}
		selectedEntities.push_back(id);
	}

	void SelectionContext::unselect(EntityId id) {
		for (int i = 0; i < selectedEntities.size(); i++) {
			if (selectedEntities[i] == id) {
				selectedEntities.erase(selectedEntities.begin() + i);
				i--;
			}
		}
	}

	void SelectionContext::unselectAll() {
		selectedEntities.clear();
	}

	bool SelectionContext::isSelected(EntityId id) {
		for (int i = 0; i < selectedEntities.size(); i++) {
			if (selectedEntities[i] == id) {
				return true;
			}
		}
		return false;
	}

	bool SelectionContext::isAnySelected() {
		return selectedEntities.size() > 0;
	}

	bool SelectionContext::isSingleSelection() {
		return selectedEntities.size() == 1;
	}

	bool SelectionContext::isMultiSelection() {
		return selectedEntities.size() > 1;
	}

	const std::vector<EntityId>& SelectionContext::getSelected() {
		return selectedEntities;
	}

}
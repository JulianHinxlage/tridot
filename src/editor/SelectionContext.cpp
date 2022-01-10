//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "SelectionContext.h"

namespace tri {

    void SelectionContext::select(EntityId id) {
        selected.insert(id);
    }

    void SelectionContext::unselect(EntityId id) {
        selected.erase(id);
    }

    void SelectionContext::unselectAll() {
        selected.clear();
    }

    bool SelectionContext::isSelected(EntityId id) {
        return selected.find(id) != selected.end();
    }

    const std::set<EntityId> &SelectionContext::getSelected() {
        return selected;
    }

}

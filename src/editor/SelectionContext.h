//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

namespace tri {

    class SelectionContext {
    public:
        void select(EntityId id);
        void unselect(EntityId id);
        void unselectAll();
        bool isSelected(EntityId id);
        const std::set<EntityId> getSelected();
    private:
        std::set<EntityId> selected;
    };

}


//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/core/config.h"
#include <map>

namespace tridot {

    class SelectionContext {
    public:
        std::map<EntityId, bool> entities;
        EntityId lastSelected;

        void select(EntityId id, bool reset = true);
        void unselect();
        void unselect(EntityId id);
        bool isSelected(EntityId id);
        EntityId getSingleSelection();

        void destroyAll();
        EntityId duplicate(EntityId id, bool addAction = true);
        void duplicateAll();
    };

}


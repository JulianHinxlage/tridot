//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_SELECTIONCONTEXT_H
#define TRIDOT_SELECTIONCONTEXT_H

#include "tridot/ecs/config.h"
#include <map>

namespace tridot {

    class SelectionContext {
    public:
        std::map<ecs::EntityId, bool> selectedEntities;
        ecs::EntityId lastSelected;

        void select(ecs::EntityId id, bool reset = true);
        void unselect();
        void unselect(ecs::EntityId id);
        bool isSelected(ecs::EntityId id);
        ecs::EntityId getSingleSelection();

        void destroyAll();
        ecs::EntityId duplicate(ecs::EntityId id, bool addAction = true);
        void duplicateAll();
    };

}

#endif //TRIDOT_SELECTIONCONTEXT_H

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_ENTITIES_PANEL_H
#define TRIDOT_ENTITIES_PANEL_H

#include "tridot/core/config.h"
#include <map>
#include <vector>

namespace tridot {

	class EntitiesPanel {
	public:
		void update();
        void updateEntity(EntityId id, bool controlDown, bool shiftDown);
        void updateEntityMenu(EntityId id);
        void newEntity(EntityId parentId = -1, bool addAction = true);
        std::map<EntityId, bool> treesOpen;
        EntityId clickedEntity = -1;
        EntityId hoveredEntity = -1;
	};

}

#endif //TRIDOT_ENTITIES_PANEL_H

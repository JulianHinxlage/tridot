//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_ENTITIES_PANEL_H
#define TRIDOT_ENTITIES_PANEL_H

#include "tridot/ecs/config.h"
#include <map>
#include <vector>

namespace tridot {

	class EntitiesPanel {
	public:
		void update();
		void updateChildren();
        void updateEntity(ecs::EntityId id, bool controlDown, bool shiftDown);
        void updateEntityMenu(ecs::EntityId id);
        std::map<ecs::EntityId, std::vector<ecs::EntityId>> children;
        std::map<ecs::EntityId, bool> treesOpen;
        ecs::EntityId clickedEntity = -1;
        ecs::EntityId hoveredEntity = -1;
	};

}

#endif //TRIDOT_ENTITIES_PANEL_H

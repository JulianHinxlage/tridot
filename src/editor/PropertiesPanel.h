//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_PROPERTIES_PANEL_H
#define TRIDOT_PROPERTIES_PANEL_H

#include "tridot/ecs/config.h"

namespace tridot {

	class PropertiesPanel {
	public:
		void update();
		void updateProperties(ecs::EntityId id);
		void updateMultiProperties();
	};

}

#endif //TRIDOT_PROPERTIES_PANEL_H

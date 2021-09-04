//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/core/config.h"

namespace tridot {

	class PropertiesPanel {
	public:
		void update();
		void updateProperties(EntityId id);
		void updateMultiProperties();
	};

}


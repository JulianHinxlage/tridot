//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "entity/World.h"

namespace tri {

	class EntityUtil {
	public:
		static void replaceIds(const std::map<EntityId, EntityId>& idMap, World* world);
	};

}
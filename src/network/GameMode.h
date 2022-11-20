//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "engine/Prefab.h"

namespace tri {

	class GameMode {
	public:
		std::vector<Ref<Prefab>> playerPrefab;
		std::vector<EntityId> localPlayer;
	};

}
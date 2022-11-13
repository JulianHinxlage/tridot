//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "core/core.h"
#include "engine/Map.h"
#include "engine/AssetManager.h"
#include "engine/RuntimeMode.h"

namespace tri {

	class MapTransition {
	public:
		Ref<Map> map;
		
		void trigger() {
			if (map) {
				auto file = env->assetManager->getFile(map);
				if (file == "") {
					file = map->file;
				}
				if (file != "") {
					RuntimeMode::Mode previousMode = env->runtimeMode->getMode();
					env->eventManager->onMapBegin.addListener([previousMode](World *world, std::string file) {
						env->runtimeMode->setMode(previousMode);
					}, true);
					Map::loadAndSetToActiveWorld(file);
				}
			}
		}
	};

	TRI_COMPONENT_CATEGORY(MapTransition, "Gameplay");
	TRI_PROPERTIES1(MapTransition, map);
	TRI_FUNCTION(MapTransition, trigger);

}

//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "entity/World.h"
#include "engine/Transform.h"
#include "engine/Camera.h"
#include "core/util/Guid.h"

namespace tri {

	class EntityUtil {
	public:
		static void replaceIds(const std::map<EntityId, EntityId>& idMap, World* world);
		static void removeEntityWithChilds(EntityId id);
		static void eachChild(EntityId id, bool recursive, const std::function<void(EntityId id)>& callback);

		template<typename Component>
		static Component *getComponentInHierarchy(EntityId id) {
			if (auto *comp = env->world->getComponent<Component>(id)) {
				return comp;
			}
			if (auto* transform = env->world->getComponent<Transform>(id)) {
				if (transform->parent != -1) {
					return getComponentInHierarchy<Component>(transform->parent);
				}
			}
			return nullptr;
		}

		static Camera* getPrimaryCamera();

		static EntityId getEntityByGuid(Guid guid);
		static EntityId getEntityByName(const std::string &name);
	};

}
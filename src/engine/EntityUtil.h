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

		static Guid getGuid(EntityId id);
		static const std::string &getName(EntityId id);

		static EntityId getEntityByGuid(Guid guid);
		static EntityId getEntityByName(const std::string &name);
		static bool isEntityOwning(EntityId id);
		static bool isEntityOwning(Guid guid);
		static void setIsEntityOwningFunction(const std::function<bool(Guid guid)> &function);

		template<typename Enum>
		static std::string enumString(Enum e) {
			for (auto& v : Reflection::getDescriptor<Enum>()->enumValues) {
				if (v.second == (int)e) {
					return v.first;
				}
			}
			return std::to_string((int)e);
		}

	};

}
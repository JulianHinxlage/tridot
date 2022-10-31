//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "entity/World.h"
#include "engine/Asset.h"
#include "entity/DynamicObjectBuffer.h"

namespace tri {

	class Prefab : public Asset {
	public:
		EntityId createEntity(World* world = nullptr, EntityId hint = -1, std::map<EntityId, EntityId>* idMap = nullptr);
		void copyEntity(EntityId id, World* world = nullptr, bool includeChilds = false);
		void copyIntoEntity(EntityId id, World* world = nullptr, bool includeChilds = false, std::map<EntityId, EntityId>* idMap = nullptr);

		template<typename T>
		T* addComponent(const T& t = T()) {
			return (T*)addComponent(Reflection::getClassId<T>(), &t);
		}
		template<typename T>
		T* getComponent() {
			return (T*)getComponent(Reflection::getClassId<T>());
		}
		template<typename T>
		void removeComponent() {
			removeComponent(Reflection::getClassId<T>());
		}

		void* addComponent(int classId, const void *ptr = nullptr);
		void* getComponent(int classId);
		void removeComponent(int classId);
		Prefab* addChild();
		void clear();
		const std::vector<DynamicObjectBuffer>& getComponents();
		const std::vector<Ref<Prefab>>& getChilds();
		EntityId getEntityId();
		void setEntityId(EntityId id);

		virtual bool load(const std::string &file);
		virtual bool save(const std::string& file);

	private:
		std::vector<DynamicObjectBuffer> components;
		std::vector<Ref<Prefab>> childs;
		EntityId entityId;
	};

}

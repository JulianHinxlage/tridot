//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/System.h"
#include "ComponentStorage.h"
#include <deque>

namespace tri {

	/*
		EntitySystem:
		( ) generation in id

		(x) add/remove entity
		(x) add/remove components
		(x) get components
		(x) get signature

		(x) pending operations

		(x) clear
		(x) copy
		swap
		save/load serialisation

		(x) onAdd/onRemove entities/components

		(x) ComponentStorage
		(x) Store use: ctor, copy ctor, move ect. to proper handle components
		(x) optimization: groups / store alignement

		( ) dynamic Component Buffer
		( ) prefab

	*/

	template<typename... Components>
	class EntityView;

	class World {
	public:
		bool enablePendingOperations = true;

		World();
		~World();

		template<typename Component>
		Component& addComponent(EntityId id) {
			return *(Component*)addComponent(id, Reflection::getClassId<Component>());
		}

		template<typename Component>
		Component& addComponent(EntityId id, const Component &comp) {
			return *(Component*)addComponent(id, Reflection::getClassId<Component>(), &comp);
		}

		template<typename... Components>
		void addComponents(EntityId id) {
			(addComponent(id, Reflection::getClassId<Components>()), ...);
		}

		template<typename... Components>
		void addComponents(EntityId id, const Components &... comps) {
			(addComponent(id, Reflection::getClassId<Components>(), &comps), ...);
		}

		template<typename... Components>
		EntityId addEntity(const Components &... comps) {
			EntityId id = addEntity();
			addComponents(id, comps...);
			return id;
		}

		template<typename Component>
		Component* getComponent(EntityId id) {
			return (Component*)getComponent(id, Reflection::getClassId<Component>());
		}

		template<typename Component>
		Component* getComponentUnchecked(EntityId id) {
			return (Component*)getComponentUnchecked(id, Reflection::getClassId<Component>());
		}

		template<typename Component>
		bool hasComponent(EntityId id) {
			return hasComponent(id, Reflection::getClassId<Component>());
		}

		template<typename Component>
		void removeComponent(EntityId id) {
			removeComponent(id, Reflection::getClassId<Component>());
		}

		template<typename Component>
		Component& getOrAddComponent(EntityId id) {
			return *(Component*)getOrAddComponent(id, Reflection::getClassId<Component>());
		}

		template<typename Component>
		EntityId getIdByComponent(Component *comp) {
			return getIdByComponent(comp, Reflection::getClassId<Component>());
		}

		template<typename... Components>
		EntitySignature createSignature() {
			EntitySignature signature = 0;
			((signature |= ((EntitySignature)1 << getComponentId(Reflection::getClassId<Components>()))), ...);
			return signature;
		}

		template<typename Component>
		ComponentStorage* getComponentStorage() {
			return getComponentStorage(Reflection::getClassId<Component>());
		}

		template<typename... Components>
		EntityView<Components...> view() {
			return EntityView<Components...>(this);
		}

		template<typename... Components, typename Func>
		void each(Func func) {
			view<Components...>().each(func);
		}

		//components that are grouped together are faster to iterate with a EntityView, when the view matches all the components
		//if group1 includes components of group2, than all components of group2 need to be included in group1
		template<typename... Components>
		void setComponentGroup() {
			TRI_ASSERT(sizeof...(Components) >= 2, "goups need at least 2 components");

			std::vector<ComponentStorage*> storages;
			((storages.push_back(getComponentStorage<Components>())), ...);
			setComponentGroup(storages);
		}


		EntityId addEntity(EntityId hint = -1);
		bool hasEntity(EntityId id);
		void removeEntity(EntityId id);

		void* addComponent(EntityId id, int classId, const void *ptr = nullptr);
		void* getComponent(EntityId id, int classId);
		void* getComponentUnchecked(EntityId id, int classId);
		bool hasComponent(EntityId id, int classId);
		void removeComponent(EntityId id, int classId);
		void* getOrAddComponent(EntityId id, int classId);
		EntityId getIdByComponent(void* comp, int classId);

		void copy(World& from);
		void clear();
		void* getComponentPending(EntityId id, int classId);
		void performePending();
		EntitySignature getSignature(EntityId id);

		ComponentStorage* getComponentStorage(int classId);
		ComponentStorage* getEntityStorage();
		void setComponentGroup(const std::vector<ComponentStorage*>& storages);
	private:
		//component data
		std::vector<std::shared_ptr<ComponentStorage>> storages;
		ComponentStorage entityStorage;

		//currently not used entity ids
		std::deque<EntityId> freeEntityIds;
		EntityId maxCurrentEntityId;

		//pending buffers
		std::vector<std::shared_ptr<ComponentStorage>> pendingAddComponentStorages;
		std::vector<std::shared_ptr<std::vector<EntityId>>> pendingRemoveComponentIds;
		std::vector<EntityId> pendingAddIds;
		std::vector<EntityId> pendingRemoveIds;

		//events buffers
		std::vector<std::shared_ptr<std::vector<EntityId>>> onComponentAddIds;
		std::vector<std::shared_ptr<std::vector<EntityId>>> onComponentRemoveIds;
		std::vector<EntityId> onEntityAddIds;
		std::vector<EntityId> onEntityRemoveIds;

		//to map classIds to bit positions in entity signatures
		std::vector<int> componentIdMap;
		int nextComponentId = 0;

		int getComponentId(int classId);
	};

}

#include "EntityView.h"

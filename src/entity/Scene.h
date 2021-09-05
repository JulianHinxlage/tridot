//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "pch.h"
#include "core/core.h"
#include "ComponentPool.h"

namespace tri {

	typedef uint64_t EntitySignatureBitmap;

	template<typename... Components>
	class EntityView;

	class Scene : public System {
	public:
		Scene();
		Scene(const Scene &scene);
		~Scene();

		EntityId addEntity();
		EntityId addEntityHinted(EntityId hint);
		bool removeEntity(EntityId id);
		bool hasEntity(EntityId id);

		template<typename... Components>
		EntityId addEntity(const Components&... components) {
			EntityId id = addEntity();
			addComponents(id, components...);
			return id;
		}

		template<typename Component, typename... Args>
		Component& addComponent(EntityId id, const Args&... args) {
			Component* component = (Component*)addComponent(env->reflection->getTypeId<Component>(), id);
			new (component) Component(args...);
			return *component;
		}

		template<typename... Components>
		void addComponents(EntityId id, const Components&... components) {
			((addComponent<Components>(id, components)), ...);
		}

		template<typename Component>
		Component& getComponent(EntityId id) {
			static int typeId = env->reflection->getTypeId<Component>();
			return *(Component*)getComponent(typeId, id);
		}

		template<typename Component>
		bool hasComponent(EntityId id) {
			return hasComponent(env->reflection->getTypeId<Component>(), id);
		}

		template<typename Component>
		bool removeComponent(EntityId id) {
			if (hasComponent<Component>(id)) {
				getComponent<Component>(id)->~Component();
			}
			return removeComponent(env->reflection->getTypeId<Component>(), id);
		}

		void* addComponent(int typeId, EntityId id);
		void* getComponent(int typeId, EntityId id);
		bool hasComponent(int typeId, EntityId id);
		bool removeComponent(int typeId, EntityId id);

		EntitySignatureBitmap& getSignature(EntityId id);
		ComponentPool* getComponentPool(int typeId);
		ComponentPool* getEntityPool();
		void clear();
		void operator=(const Scene& scene);
		void update() override;
		void enablePendingOperations(bool enable);
		
		template<typename Component>
		ComponentPool* getComponentPool() {
			return getComponentPool(env->reflection->getTypeId<Component>());
		}

		template<typename... Components>
		EntitySignatureBitmap createSignature() {
			EntitySignatureBitmap signature = 0;
			((signature |= (1 << env->reflection->getTypeId<Components>())) , ...);
			return signature;
		}

		template<typename Component>
		EntityId getEntityIdByComponent(const Component &comp) {
			auto* pool = getComponentPool<Component>();
			EntityId index = &comp - (Component*)pool->data();
			if (index < pool->size()) {
				return pool->getIdByIndex(index);
			}
			else {
				return -1;
			}
		}

		template<typename... Components>
		EntityView<Components...> view() {
			return EntityView<Components...>(this);
		}

	private:
		std::vector<std::shared_ptr<ComponentPool>> pools;
		ComponentPool entityPool;
		std::unordered_set<EntityId> freeList;

		class PendingOperation {
		public:
			EntityId id;
			int typeId;
			bool isAddOperation;
			std::vector<uint8_t> data;
		};
		bool pendingOperationsEnabled;
		std::vector<PendingOperation> pendingOperations;
	};

}

#include "EntityView.h"

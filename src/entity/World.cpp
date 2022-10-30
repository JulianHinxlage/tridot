//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "World.h"
#include "core/Environment.h"

namespace tri {

	class WorldManager : public System {
	public:
		int postTickListener = 0;

		void shutdown() {
			delete env->world;
			worlds.clear();
			env->world = nullptr;
			env->eventManager->postTick.removeListener(postTickListener);
		}

		void init() override {
			env->world = new World();
			postTickListener = env->eventManager->postTick.addListener([]() {
				if (env->world) {
					env->world->performePending();
				}
			});
		}

		void addWorld(World* world) {
			worlds.push_back(world);
		}

		void removeWorld(World* world) {
			for (int i = 0; i < worlds.size(); i++) {
				if(worlds[i] == world) {
					worlds.erase(worlds.begin() + i);
					break;
				}
			}
		}

		std::vector<World*> worlds;
	};
	TRI_SYSTEM(WorldManager);

	const std::vector<World*>& World::getAllWorlds() {
		return env->systemManager->getSystem<WorldManager>()->worlds;
	}

	World::World()
		: entityStorage(Reflection::getClassId<EntitySignature>()) {
		maxCurrentEntityId = 0;
		nextComponentId = 0;
		env->systemManager->getSystem<WorldManager>()->addWorld(this);
	}

	World::~World() {
		env->systemManager->getSystem<WorldManager>()->removeWorld(this);
	}

	EntityId World::addEntity(EntityId hint) {
		EntityId id = hint;
		if (hint == -1 || hasEntity(hint)) {
			do {
				if (!freeEntityIds.empty()) {
					id = freeEntityIds.front();
					freeEntityIds.pop_front();
				}
				else {
					id = maxCurrentEntityId++;
					break;
				}
			} while (id >= maxCurrentEntityId || hasEntity(id));
		}

		if (enablePendingOperations) {
			pendingAddIds.push_back(id);
			return id;
		}
		else {
			onEntityAddIds.push_back(id);
		}

		if (id >= maxCurrentEntityId) {
			for (int i = maxCurrentEntityId; i < id; i++) {
				maxCurrentEntityId++;
				freeEntityIds.push_back(i);
			}
			maxCurrentEntityId++;
		}

		TRI_ASSERT(!entityStorage.hasComponent(id), "entity id already in use");
#if TRI_DEBUG
		for (auto& store : storages) {
			if (store) {
				TRI_ASSERT(!store->hasComponent(id), "entity has already a component attached");
			}
		}
#endif

		EntitySignature &signature = *(EntitySignature*)entityStorage.addComponent(id);
		signature = 0;
		return id;
	}

	bool World::hasEntity(EntityId id) {
		return entityStorage.hasComponent(id);
	}

	void World::removeEntity(EntityId id) {
		if (hasEntity(id)) {
			if (enablePendingOperations) {
				pendingRemoveIds.push_back(id);
				return;
			}
			else {
				onEntityRemoveIds.push_back(id);
			}

			entityStorage.removeComponent(id);
			if (id >= maxCurrentEntityId - 1) {
				while (maxCurrentEntityId > 0 && !entityStorage.hasComponent(maxCurrentEntityId - 1)) {
					maxCurrentEntityId--;
				}
			}
			else {
				freeEntityIds.push_back(id);
			}

			for (auto& store : storages) {
				if (store) {
					if (store->hasComponent(id)) {
						store->removeComponent(id);
					}
				}
			}
		}
	}

	void* World::addComponent(EntityId id, int classId, const void* ptr) {
		if (enablePendingOperations) {
			//pending operation
			if (pendingAddComponentStorages.size() <= classId) {
				pendingAddComponentStorages.resize(classId + 1);
			}
			if (!pendingAddComponentStorages[classId]) {
				pendingAddComponentStorages[classId] = std::make_shared<ComponentStorage>(classId);
			}
			return pendingAddComponentStorages[classId]->addComponent(id, ptr);
		}
		else {
			//event buffer
			if (onComponentAddIds.size() <= classId) {
				onComponentAddIds.resize(classId + 1);
			}
			if (!onComponentAddIds[classId]) {
				onComponentAddIds[classId] = std::make_shared<std::vector<EntityId>>();
			}
			onComponentAddIds[classId]->push_back(id);
		}


		if (storages.size() <= classId) {
			storages.resize(classId + 1);
		}
		if (!storages[classId]) {
			storages[classId] = std::make_shared<ComponentStorage>(classId);
		}
		auto* signature = (EntitySignature*)entityStorage.getComponentById(id);
		*signature |= ((EntitySignature)1 << getComponentId(classId));
		return storages[classId]->addComponent(id, ptr);
	}

	void* World::getComponent(EntityId id, int classId) {
		if (storages.size() <= classId) {
			return nullptr;
		}
		if (!storages[classId]) {
			return nullptr;
		}
		return storages[classId]->getComponentById(id);
	}

	void* World::getComponentUnchecked(EntityId id, int classId) {
		return storages[classId]->getComponentByIdUnchecked(id);
	}

	bool World::hasComponent(EntityId id, int classId) {
		if (storages.size() <= classId) {
			return false;
		}
		if (!storages[classId]) {
			return false;
		}
		return storages[classId]->hasComponent(id);
	}

	void World::removeComponent(EntityId id, int classId) {
		if (enablePendingOperations) {
			//pending operation
			if (pendingRemoveComponentIds.size() <= classId) {
				pendingRemoveComponentIds.resize(classId + 1);
			}
			if (!pendingRemoveComponentIds[classId]) {
				pendingRemoveComponentIds[classId] = std::make_shared<std::vector<EntityId>>();
			}
			auto* ids = pendingRemoveComponentIds[classId].get();
			ids->push_back(id);
			return;
		}
		else {
			//event buffer
			if (onComponentRemoveIds.size() <= classId) {
				onComponentRemoveIds.resize(classId + 1);
			}
			if (!onComponentRemoveIds[classId]) {
				onComponentRemoveIds[classId] = std::make_shared<std::vector<EntityId>>();
			}
			onComponentRemoveIds[classId]->push_back(id);
		}

		auto* signature = (EntitySignature*)entityStorage.getComponentById(id);
		*signature &= ~((EntitySignature)1 << getComponentId(classId));
		return storages[classId]->removeComponent(id);
	}

	void* World::getOrAddComponent(EntityId id, int classId) {
		if (hasComponent(id, classId)) {
			return getComponent(id, classId);
		}
		else {
			return addComponent(id, classId);
		}
	}

	EntityId World::getIdByComponent(const void* comp, int classId) {
		if (storages.size() <= classId) {
			return -1;
		}
		if (!storages[classId]) {
			return -1;
		}
		return storages[classId]->getIdByComponent(comp);
	}

	void World::copy(World& from) {
		performePending();
		from.performePending();

		nextComponentId = from.nextComponentId;
		componentIdMap = from.componentIdMap;
		freeEntityIds = from.freeEntityIds;
		maxCurrentEntityId = from.maxCurrentEntityId;
		entityStorage.copy(from.entityStorage);

		storages.resize(from.storages.size());
		for (int i = 0; i < storages.size(); i++) {
			if (from.storages[i]) {
				storages[i] = std::make_shared<ComponentStorage>(from.storages[i]->classId);
				storages[i]->copy(*from.storages[i]);
			}
			else if (storages[i]) {
				storages[i] = nullptr;
			}
		}
	}

	void* World::getComponentPending(EntityId id, int classId) {
		if (enablePendingOperations) {
			if (pendingAddComponentStorages.size() <= classId) {
				return nullptr;
			}
			if (!pendingAddComponentStorages[classId]) {
				return nullptr;
			}
			return pendingAddComponentStorages[classId]->getComponentById(id);
		}else{
			return getComponent(id, classId);
		}
	}

	void World::clear() {
		entityStorage.clear();
		freeEntityIds.clear();
		maxCurrentEntityId = 0;
		for (auto& store : storages) {
			if (store) {
				store->clear();
			}
		}
	}

	void World::performePending() {
		TRI_PROFILE_FUNC();

		bool currentEnablePendingOperations = enablePendingOperations;
		enablePendingOperations = false;


		//event buffers from non pending operations
		for (auto& id : onEntityRemoveIds) {
			env->eventManager->onEntityRemove.invoke(this, id);
		}
		for (int classId = 0; classId < onComponentRemoveIds.size(); classId++) {
			auto& ids = onComponentRemoveIds[classId];
			auto& event = env->eventManager->onComponentRemove(classId);
			if (ids) {
				for (auto& id : *ids) {
					event.invoke(this, id);
				}
			}
		}
		for (auto& id : onEntityAddIds) {
			env->eventManager->onEntityAdd.invoke(this, id);
		}
		for (int classId = 0; classId < onComponentAddIds.size(); classId++) {
			auto& ids = onComponentAddIds[classId];
			auto& event = env->eventManager->onComponentAdd(classId);
			if (ids) {
				for (auto& id : *ids) {
					event.invoke(this, id);
				}
			}
		}



		//pending remove operations
		for (auto& id : pendingRemoveIds) {
			env->eventManager->onEntityRemove.invoke(this, id);
		}
		for (auto& id : pendingRemoveIds) {
			removeEntity(id);
		}

		//pending remove operations events
		for (int classId = 0; classId < pendingRemoveComponentIds.size(); classId++) {
			auto& ids = pendingRemoveComponentIds[classId];
			auto& event = env->eventManager->onComponentRemove(classId);
			if (ids) {
				for (auto& id : *ids) {
					event.invoke(this, id);
				}
			}
		}
		for (int classId = 0; classId < pendingRemoveComponentIds.size(); classId++) {
			auto &ids = pendingRemoveComponentIds[classId];
			if (ids) {
				for (auto& id : *ids) {
					removeComponent(id, classId);
				}
			}
		}



		//pending add operations
		for (auto& id : pendingAddIds) {
			addEntity(id);
		}
		for (int classId = 0; classId < pendingAddComponentStorages.size(); classId++) {
			auto& store = pendingAddComponentStorages[classId];
			if (store) {
				auto* desc = Reflection::getDescriptor(classId);
				for (int i = 0; i < store->size(); i++) {
					void* ptr = addComponent(store->getIdByIndex(i), classId);
					desc->copy(store->getComponentByIndex(i), ptr);
				}
			}
		}

		//pending add operations events
		for (auto& id : pendingAddIds) {
			env->eventManager->onEntityAdd.invoke(this, id);
		}
		for (int classId = 0; classId < pendingAddComponentStorages.size(); classId++) {
			auto& store = pendingAddComponentStorages[classId];
			auto& event = env->eventManager->onComponentAdd(classId);
			if (store) {
				for (int i = 0; i < store->size(); i++) {
					event.invoke(this, store->getIdByIndex(i));
				}
			}
		}



		//clear buffers
		pendingRemoveIds.clear();
		pendingAddIds.clear();
		onEntityRemoveIds.clear();
		onEntityAddIds.clear();

		for (int classId = 0; classId < onComponentRemoveIds.size(); classId++) {
			auto& ids = onComponentRemoveIds[classId];
			if (ids) {
				ids->clear();
			}
		}
		for (int classId = 0; classId < onComponentAddIds.size(); classId++) {
			auto& ids = onComponentAddIds[classId];
			if (ids) {
				ids->clear();
			}
		}
		for (int classId = 0; classId < pendingRemoveComponentIds.size(); classId++) {
			auto& ids = pendingRemoveComponentIds[classId];
			if (ids) {
				ids->clear();
			}
		}
		for (int classId = 0; classId < pendingAddComponentStorages.size(); classId++) {
			auto& store = pendingAddComponentStorages[classId];
			if (store) {
				store->clear();
			}
		}


		enablePendingOperations = currentEnablePendingOperations;
	}

	EntitySignature World::getSignature(EntityId id){
		return *(EntitySignature*)entityStorage.getComponentById(id);
	}

	ComponentStorage* World::getComponentStorage(int classId) {
		if (storages.size() <= classId) {
			if (enablePendingOperations) {
				//resizing while an other thread is accessing a store can cause a race condition on the stores array, so only resize when pending is disabled
				// => maybe add store requests as pending operation?
				return nullptr;
			}
			else {
				storages.resize(classId + 1);
			}
		}
		if (!storages[classId]) {
			storages[classId] = std::make_shared<ComponentStorage>(classId);
		}
		return storages[classId].get();
	}

	ComponentStorage* World::getEntityStorage() {
		return &entityStorage;
	}

	int World::getComponentId(int classId) {
		if (componentIdMap.size() <= classId) {
			componentIdMap.resize(classId + 1, -1);
		}
		int id = componentIdMap[classId];
		if (id == -1) {
			id = nextComponentId++;
			componentIdMap[classId] = id;
		}
		return id;
	}

	void World::setComponentGroup(const std::vector<ComponentStorage*>& storages) {
		return;
		auto group = std::make_shared<ComponentStorage::Group>();
		group->storages = storages;
		group->size = 0;

		bool conflict = false;
		for (int i = 0; i < storages.size(); i++) {
			auto* storage = storages[i];
			if (storage->checkGroup(group) == 2) {
				conflict = true;
			}
		}
		TRI_ASSERT(!conflict, "conflicting group found");

		if (!conflict) {
			for (int i = 0; i < storages.size(); i++) {
				auto* storage = storages[i];
				storage->addGroup(group);
			}
		}
	}

	void World::removeComponentStorage(int classId) {
		if (storages.size() > classId) {
			storages[classId] = nullptr;
		}
		if (pendingAddComponentStorages.size() > classId) {
			pendingAddComponentStorages[classId] = nullptr;
		}
		int compId = getComponentId(classId);
		int size = entityStorage.size();
		EntitySignature* data = (EntitySignature*)entityStorage.getComponentData();
		for (int i = 0; i < size; i++) {
			data[i] &= ~(1 << compId);
		}
	}

}

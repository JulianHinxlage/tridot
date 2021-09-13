//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Scene.h"

namespace tri {

    TRI_REGISTER_SYSTEM_INSTANCE(Scene, env->scene);

    Scene::Scene() {
        entityPool = ComponentPool(env->reflection->getTypeId<EntitySignatureBitmap>(), sizeof(EntitySignatureBitmap));
        pendingOperationsEnabled = true;
        addPendingOperations = true;
    }

    Scene::Scene(const Scene& scene) {
        operator=(scene);
    }

    Scene::~Scene() {
        clear();
    }

    EntityId Scene::addEntity() {
        return addEntityHinted(-1);
    }

    EntityId Scene::addEntityHinted(EntityId hint) {
        if (hint != -1) {
            if (freeList.contains(hint)) {
                freeList.erase(hint);
            }
            else if (hint < (EntityId)entityPool.size()) {
                hint = -1;
            }
        }
        if (hint == -1) {
            if (freeList.empty()) {
                hint = entityPool.size();
            }
            else {
                for (EntityId id : freeList) {
                    hint = id;
                    break;
                }
                freeList.erase(hint);
            }
        }
        //todo: keep track of free ids if entities are created with a hint
        while(entityPool.has(hint)){
            hint++;
        }
        entityPool.add(hint);
        getSignature(hint) = 0;
        return hint;
    }

    bool Scene::removeEntity(EntityId id) {
        if (entityPool.remove(id)) {
            for (auto& pool : pools) {
                if (pool) {
                    pool->remove(id);
                }
            }
            freeList.insert(id);
            return true;
        }
        else {
            return false;
        }
    }

    bool Scene::hasEntity(EntityId id) {
        return entityPool.has(id);
    }

    void* Scene::addComponent(int typeId, EntityId id) {
        if (addPendingOperations) {
            bool pending = pendingOperationsEnabled && !hasComponent(typeId, id);
            pendingOperations.push_back({id, typeId, true, pending});
            if (pending) {
                pendingOperations.back().data.resize(env->reflection->getType(typeId)->size);
                return (void*)pendingOperations.back().data.data();
            }
        }
        auto* pool = getComponentPool(typeId);
        getSignature(id) |= ((EntitySignatureBitmap)1 << typeId);
        return pool->add(id);
    }

    void* Scene::getComponent(int typeId, EntityId id) {
        TRI_ASSERT(pools.size() > typeId && pools[typeId] != nullptr, "unknown component");
        return pools[typeId]->getElementById(id);
    }

    bool Scene::hasComponent(int typeId, EntityId id) {
        if (pools.size() > typeId && pools[typeId] != nullptr) {
            return pools[typeId]->has(id);
        }
        else {
            return false;
        }
    }

    bool Scene::removeComponent(int typeId, EntityId id) {
        if (pools.size() > typeId && pools[typeId] != nullptr) {
            if (addPendingOperations) {
                pendingOperations.push_back({id, typeId, false, pendingOperationsEnabled});
                if (pendingOperationsEnabled) {
                    return pools[typeId]->has(id);
                }
            }
            getSignature(id) &= ~((EntitySignatureBitmap)1 << typeId);
            return pools[typeId]->remove(id);
        }
        else {
            return false;
        }
    }

    EntitySignatureBitmap& Scene::getSignature(EntityId id) {
        return *(EntitySignatureBitmap*)entityPool.getElementById(id);
    }

    ComponentPool* Scene::getComponentPool(int typeId){
        if (pools.size() <= typeId) {
            pools.resize((size_t)typeId + 1);
        }
        if (pools[typeId] == nullptr) {
            pools[typeId] = std::make_shared<ComponentPool>(typeId, env->reflection->getType(typeId)->size);
        }
        return pools[typeId].get();
    }

    ComponentPool* Scene::getEntityPool(){
        return &entityPool;
    }

    void Scene::clear() {
        pools.clear();
        entityPool.clear();
        freeList.clear();
        pendingOperations.clear();
    }

    void Scene::operator=(const Scene& scene) {
        freeList = scene.freeList;
        entityPool = scene.entityPool;
        pools.resize(scene.pools.size());
        for (int i = 0; i < pools.size(); i++) {
            if (scene.pools[i]) {
                pools[i] = std::make_shared<ComponentPool>(*scene.pools[i]);
            }
        }
    }

    void Scene::update() {
        if (pendingOperations.size() > 0) {
            std::vector<PendingOperation> ops;
            ops.swap(pendingOperations);

            for (auto& op : ops) {
                if (!op.isAddOperation) {
                    env->signals->getComponentShutdown(op.typeId).invoke(op.id, this);
                }
            }

            addPendingOperations = false;
            for (auto& op : ops) {
                if (op.isPending) {
                    if (op.isAddOperation) {
                        if (entityPool.has(op.id)) {
                            uint8_t* data = (uint8_t*)addComponent(op.typeId, op.id);
                            for (int i = 0; i < op.data.size(); i++) {
                                data[i] = op.data[i];
                            }
                        }
                    }
                    else {
                        removeComponent(op.typeId, op.id);
                    }
                }
            }
            addPendingOperations = true;

            for (auto& op : ops) {
                if (op.isAddOperation) {
                    env->signals->getComponentInit(op.typeId).invoke(op.id, this);
                }
            }
        }
    }

    void Scene::enablePendingOperations(bool enable){
        pendingOperationsEnabled = enable;
    }

}

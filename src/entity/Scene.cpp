//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Scene.h"
#include "engine/Serializer.h"
#include "engine/AssetManager.h"
#include "engine/RuntimeMode.h"

namespace tri {

    TRI_REGISTER_ASSET(Scene);

    TRI_REGISTER_SYSTEM_INSTANCE(Scene, env->scene);

    Scene::Scene() {
        file = "";
        entityPool = ComponentPool(env->reflection->getTypeId<EntitySignatureBitmap>(), sizeof(EntitySignatureBitmap));
        pendingOperationsEnabled = true;
        addPendingOperations = true;
    }

    Scene::Scene(const Scene& scene) {
        file = "";
        pendingOperationsEnabled = true;
        addPendingOperations = true;
        copy(scene);
    }

    Scene::~Scene() {
        clear();
    }

    EntityId Scene::addEntity() {
        return addEntityHinted(-1);
    }

    EntityId Scene::addEntityHinted(EntityId hint) {
        if (hint != -1) {
            if (freeList.find(hint) != freeList.end()) {
                freeList.erase(hint);
            }
            else if (hint < (EntityId)entityPool.size()) {
                if (entityPool.has(hint)) {
                    hint = -1;
                }
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
        freeList.erase(hint);

        //todo: fix components existing if entity is not
        for (int i = 0; i < pools.size(); i++) {
            auto& pool = pools[i];
            if (pool) {
                if (pool->has(hint)) {
                    pool->remove(hint);
                }
            }
        }

        env->signals->entityCreate.invoke(hint, this);
        return hint;
    }

    bool Scene::removeEntity(EntityId id) {
        if (entityPool.remove(id)) {
            env->signals->entityRemove.invoke(id, this);
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
                pendingOperations.back().component.set(typeId);
                return (void*)pendingOperations.back().component.get();
            }
        }
        auto* pool = getComponentPool(typeId);
        getSignature(id) |= ((EntitySignatureBitmap)1 << typeId);
        void *comp = pool->add(id);
        return comp;
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

    void* Scene::getPendingComponent(int typeId, EntityId id) {
        for (int i = pendingOperations.size() - 1; i >= 0; i--) {
            auto& op = pendingOperations[i];
            if (op.id != id) {
                break;
            }
            if (op.isPending) {
                if (op.isAddOperation) {
                    if (op.typeId == typeId) {
                        return op.component.get();
                    }
                }
            }
        }
        return nullptr;
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

    void Scene::clearComponent(int typeId) {
        if (pools.size() > typeId) {
            pools[typeId] = nullptr;
        }
    }

    void Scene::copy(const Scene &from) {
        file = from.file;
        freeList = from.freeList;
        entityPool.copy(from.entityPool);
        pools.resize(from.pools.size());
        for (int i = 0; i < pools.size(); i++) {
            auto& pool = from.pools[i];
            if (pool) {
                pool->lock();
                pool->unlock();
                pools[i] = std::make_shared<ComponentPool>(*pool);
            }
        }
    }

    void Scene::swap(Scene &scene) {
        freeList.swap(scene.freeList);
        pools.swap(scene.pools);
        entityPool.swap(scene.entityPool);
        std::swap(file, scene.file);
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
                            op.component.get(data);
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

    bool Scene::load(const std::string &file) {
        Clock clock;
        clear();
        if(!env->serializer->deserializeScene(file, *this)){
            return false;
        }
        this->file = file;
        update();
        env->signals->sceneLoad.invoke(this);
        env->console->info("loaded scene ", file, " in ", clock.elapsed(), "s");
        return true;
    }

    bool Scene::save(const std::string &file) {
        Clock clock;
        if(!env->serializer->serializeScene(file, *this)){
            return false;
        }
        env->console->info("saved scene ", file, " in ", clock.elapsed(), "s");
        return true;
    }

    void Scene::loadMainScene(const std::string &file) {
        env->assets->unload(file);
        if (env->editor) {
            if (env->runtime->getMode() == RuntimeMode::RUNTIME
                || env->runtime->getMode() == RuntimeMode::PAUSE) {
                env->runtime->setMode(RuntimeMode::EDIT);
            }
        }
        env->assets->get<Scene>(file, false, nullptr, [](Ref<Asset> asset){
            if (env->editor) {
                if (env->runtime->getMode() == RuntimeMode::RUNTIME
                    || env->runtime->getMode() == RuntimeMode::PAUSE) {
                    env->runtime->setMode(RuntimeMode::EDIT);
                }
            }
            env->scene->swap(*(Scene*)asset.get());
            env->signals->sceneLoad.invoke(env->scene);
            //env->assets->unload(env->scene->file);
            return true;
        });
    }

}

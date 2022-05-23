//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Scene.h"
#include "engine/Serializer.h"
#include "engine/AssetManager.h"
#include "engine/RuntimeMode.h"
#include "render/RenderThread.h"

namespace tri {

    TRI_REGISTER_ASSET(Scene);

    TRI_REGISTER_SYSTEM_INSTANCE(Scene, env->scene);

    Scene::Scene() {
        file = "";
        entityPool = ComponentPool(env->reflection->getTypeId<EntitySignatureBitmap>(), sizeof(EntitySignatureBitmap));
        pendingOperationsEnabled = true;
        addPendingOperations = true;
        maxCurrentEntityId = 0;
    }

    Scene::Scene(const Scene& scene) {
        file = "";
        pendingOperationsEnabled = true;
        addPendingOperations = true;
        maxCurrentEntityId = 0;
        copy(scene);
    }

    Scene::~Scene() {
        clear();
    }

    EntityId Scene::addEntity() {
        return addEntityHinted(-1);
    }

    EntityId Scene::addEntityHinted(EntityId hint) {
        EntityId id = hint;
        if (hint != -1) {
            //check if hint id is available
            if (freeList.contains(id)) {
                freeList.erase(id);
            }
            else {
                if (id >= maxCurrentEntityId) {
                    for (EntityId i = maxCurrentEntityId; i < id; i++) {
                        freeList.insert(i);
                    }
                    maxCurrentEntityId = id + 1;
                }
                else {
                    //requested hint id not available
                    id = -1;
                }
            }
        }
        if (id == -1) {
            if (freeList.empty()) {
                id = maxCurrentEntityId++;
            }
            else {
                //get id from free list
                for (EntityId freeId : freeList) {
                    id = freeId;
                    break;
                }
                freeList.erase(id);
            }
        }

        TRI_ASSERT(!entityPool.has(id), "entity id already in use");
#if TRI_DEBUG
        for (auto& pool : pools) {
            if (pool) {
                TRI_ASSERT(!pool->has(id), "entity has already a component attached");
            }
        }
#endif

        entityPool.add(id);
        getSignature(id) = 0;
        env->signals->entityCreate.invoke(id, this);
        return id;
    }

    bool Scene::removeEntity(EntityId id) {
        if (entityPool.remove(id)) {
            env->signals->entityRemove.invoke(id, this);
            for (auto& pool : pools) {
                if (pool) {
                    pool->remove(id);
                }
            }
            if (id == maxCurrentEntityId - 1) {
                maxCurrentEntityId--;
                while (freeList.contains(maxCurrentEntityId - 1)) {
                    maxCurrentEntityId--;
                    freeList.erase(maxCurrentEntityId);
                }
            }
            else {
                freeList.insert(id);
            }
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
                continue;
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
        maxCurrentEntityId = 0;
    }

    void Scene::clearComponent(int typeId) {
        if (pools.size() > typeId) {
            pools[typeId] = nullptr;
        }
    }

    void Scene::copy(const Scene &from) {
        file = from.file;
        freeList = from.freeList;
        maxCurrentEntityId = from.maxCurrentEntityId;
        entityPool.copy(from.entityPool);
        
        pools.clear();
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
        std::swap(maxCurrentEntityId, scene.maxCurrentEntityId);
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
        //this sould be solved differently
        env->jobSystem->addTask([file]() {

            env->assets->unload(file);
            RuntimeMode::Mode mode = env->runtime->getMode();
            if (env->editor) {
                if (mode == RuntimeMode::RUNTIME || mode == RuntimeMode::PAUSE) {
                    env->runtime->setMode(RuntimeMode::EDIT);
                }
            }
            env->assets->get<Scene>(file, AssetManager::NONE, nullptr, [file, mode](Ref<Asset> asset) {

                env->jobSystem->addTask([asset, file, mode]() {
                    if (env->editor) {
                        RuntimeMode::Mode currentMode = env->runtime->getMode();
                        if (currentMode == RuntimeMode::RUNTIME || currentMode == RuntimeMode::PAUSE) {
                            env->runtime->setMode(RuntimeMode::EDIT);
                        }
                    }

                    env->signals->sceneEnd.invoke(env->scene);
                    env->scene->swap(*(Scene*)asset.get());
                    env->signals->sceneLoad.invoke(env->scene);
                    env->signals->sceneBegin.invoke(env->scene);

                    env->renderThread->addTask([file, asset, mode]() {
                        (*(Scene*)asset.get()).clear();

                        env->jobSystem->addTask([mode]() {
                            if (env->editor) {
                                if (mode == RuntimeMode::RUNTIME || mode == RuntimeMode::PAUSE) {
                                    env->runtime->setMode(mode);
                                }
                            }
                        });
                    });

                });
                return true;
            });
        });
    }

}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_REGISTRY_H
#define TRIDOT_REGISTRY_H

#include "TypeMap.h"
#include "Pool.h"
#include "ComponentPool.h"
#include "ComponentRegister.h"
#include <cstdint>
#include <map>
#include <functional>

namespace tridot {

    template<typename... Components>
    class View;

    class Registry {
    public:
        Registry(){
            nextEntityId = 0;
            entityPool.registry = this;

            componentPools.resize(ComponentRegister::componentPools.size());
            for(int cid = 0; cid < ComponentRegister::componentPools.size(); cid++){
                setupPool(cid);
            }

            onRegisterCallbackId = ComponentRegister::onRegister().add([this](int typeId){
                uint32_t cid = ComponentRegister::id(typeId);
                setupPool(cid);
            }, "Registry");

            onUnregisterCallbackId = ComponentRegister::onUnregister().add([this](int typeId){
                uint32_t cid = ComponentRegister::id(typeId);
                if(cid < componentPools.size()){
                    componentPools[cid] = nullptr;
                }
            }, "Registry");
        }

        ~Registry(){
            ComponentRegister::onRegister().remove(onRegisterCallbackId);
            ComponentRegister::onUnregister().remove(onUnregisterCallbackId);
        }

        EntityId createHinted(EntityId hint){
            EntityId id = -1;
            if(hint != -1){
                if(!entityPool.has(hint)){
                    id = hint;
                    auto entry = freeEntityIds.find(hint);
                    if(entry != freeEntityIds.end()){
                        freeEntityIds.erase(entry);
                    }
                }
            }
            if(id == -1){
                if(!freeEntityIds.empty()){
                    id = freeEntityIds.begin()->first;
                    freeEntityIds.erase(freeEntityIds.begin());
                }else{
                    while(entityPool.has(nextEntityId)){
                        nextEntityId++;
                    }
                    id = nextEntityId++;
                }
            }
            entityPool.add(id, 0);
            return id;
        }

        EntityId create() {
            return createHinted(-1);
        }

        template<typename... Components>
        EntityId create(const Components&... comps){
            EntityId id = create();
            add(id, comps...);
            return id;
        }

        void destroy(EntityId id){
            if(!entityPool.has(id)){
                return;
            }
            for(auto &pool : componentPools){
                if(pool != nullptr){
                    if(pool->has(id)){
                        pool->remove(id);
                    }
                }
            }
            entityPool.remove(id);
            if(id == nextEntityId - 1){
                while(nextEntityId != 0 && !entityPool.has(nextEntityId - 1)){
                    nextEntityId--;
                    auto entry = freeEntityIds.find(nextEntityId);
                    if(entry != freeEntityIds.end()){
                        freeEntityIds.erase(entry);
                    }
                }
            }else{
                freeEntityIds[id] = true;
            }
        }

        bool exists(EntityId id){
            return entityPool.has(id);
        }

        template<typename Component, typename... Args>
        Component &add(EntityId id, Args &&... args){
            auto &pool = getPool<Component>();
            uint32_t index = pool.add(id, std::forward<Args>(args)...);
            return *(Component*)pool.get(index);
        }

        template<typename... Components>
        void add(EntityId id, const Components&... comps){
            (add<Components, const Components &>(id, comps) , ...);
        }

        template<typename Component>
        Component &get(EntityId id){
            uint32_t cid = ComponentRegister::id<Component>();
            TRI_ASSERT(cid < componentPools.size(), "index out of bounds")
            TRI_ASSERT(componentPools[cid] != nullptr, "component pool not present")
            return *(Component*)componentPools[cid]->getById(id);
        }

        template<typename... Components>
        void remove(EntityId id){
            (getPool<Components>().remove(id) , ...);
        }

        template<typename Component>
        bool has(EntityId id){
            uint32_t cid = ComponentRegister::id<Component>();
            if(cid >= componentPools.size()){
                return false;
            }
            if(componentPools[cid] == nullptr){
                return false;
            }
            return componentPools[cid]->has(id);
        }

        SignatureBitMap getSignature(EntityId id){
            return *(SignatureBitMap*)entityPool.getById(id);
        }

        template<typename... Components>
        SignatureBitMap createSignature(){
            if constexpr (sizeof...(Components) != 0) {
                TRI_ASSERT(!((ComponentRegister::id<Components>() >= (sizeof(SignatureBitMap) * 8)) || ...),"to many component types")
                return ((SignatureBitMap(1) << ComponentRegister::id<Components>()) | ...);
            }else{
                return SignatureBitMap(0);
            }
        }

        template<typename... Components>
        bool hasAll(EntityId id){
            return (has<Components>(id) && ...);
        }

        template<typename... Components>
        bool hasAny(EntityId id){
            return (has<Components>(id) || ...);
        }

        bool hasAll(EntityId id, SignatureBitMap sig){
            return (getSignature(id) & sig) == sig;
        }

        bool hasAny(EntityId id, SignatureBitMap sig){
            return (getSignature(id) & sig) != 0;
        }

        template<typename... Components>
        View<Components...> view(){
            return View<Components...>(this);
        }

        template<typename... Components, typename Func>
        void each(const Func &func){
            view<Components...>().each(func);
        }

        auto onCreate(){
            return entityPool.onAdd();
        }

        auto onDestroy(){
            return entityPool.onRemove();
        }

        template<typename Component>
        auto onAdd(){
            return getPool<Component>().onAdd();
        }

        template<typename Component>
        auto onRemove(){
            return getPool<Component>().onRemove();
        }

        void clear(){
            freeEntityIds.clear();
            nextEntityId = 0;
            entityPool.clear();
            for(auto &pool : componentPools){
                if(pool != nullptr){
                    pool->clear();
                }
            }
        }

        void copy(const Registry &source){
            freeEntityIds = source.freeEntityIds;
            nextEntityId = source.nextEntityId;
            entityPool.copy(source.entityPool);
            entityPool.registry = this;
            componentPools.resize(source.componentPools.size());
            for(int cid = 0; cid < source.componentPools.size(); cid++){
                if(source.componentPools[cid]){
                    componentPools[cid] = source.componentPools[cid]->make();
                    componentPools[cid]->copy(*source.componentPools[cid]);
                    componentPools[cid]->registry = this;
                    componentPools[cid]->onAdd().add([cid](Registry *reg, EntityId id) {
                        *(SignatureBitMap *) reg->entityPool.getById(id) |= (SignatureBitMap(1) << cid);
                    });
                    componentPools[cid]->onRemove().add([cid](Registry *reg, EntityId id) {
                        *(SignatureBitMap *) reg->entityPool.getById(id) &= ~(SignatureBitMap(1) << cid);
                    });
                }else{
                    componentPools[cid] = nullptr;
                }
            }
        }

        void swap(Registry &other){
            componentPools.swap(other.componentPools);
            entityPool.swap(other.entityPool);
            freeEntityIds.swap(other.freeEntityIds);
            std::swap(nextEntityId, other.nextEntityId);

            entityPool.registry = this;
            other.entityPool.registry = &other;
            for(int cid = 0; cid < componentPools.size(); cid++){
                if(componentPools[cid] != nullptr){
                    componentPools[cid]->registry = this;
                }
                if(cid < other.componentPools.size() && other.componentPools[cid] != nullptr){
                    other.componentPools[cid]->registry = &other;
                }
            }
        }

        Pool &getEntityPool(){
            return entityPool;
        }

        template<typename Component>
        ComponentPool<Component> &getPool(){
            uint32_t cid = ComponentRegister::id<Component>();
            while(cid >= componentPools.size()){
                componentPools.push_back(nullptr);
            }
            if(componentPools[cid] == nullptr){
                ComponentRegister::registerComponent<Component>();
                setupPool(cid);
            }
            return *(ComponentPool<Component>*)componentPools[cid].get();
        }

        Pool *getPool(int typeId){
            int cid = ComponentRegister::id(typeId);
            if(cid < 0 || cid >= componentPools.size()){
                return nullptr;
            }else{
                return componentPools[cid].get();
            }
        }

        void *get(int id, int typeId){
            auto *pool = getPool(typeId);
            if (pool && pool->has(id)) {
                return pool->getById(id);
            }else{
                return nullptr;
            }
        }

        void *addByTypeId(int id, int typeId){
            auto *pool = getPool(typeId);
            if (pool) {
                uint32_t index = pool->add(id, nullptr);
                return pool->get(index);
            }else{
                return nullptr;
            }
        }

        void remove(int id, int typeId){
            auto *pool = getPool(typeId);
            if (pool && pool->has(id)) {
                pool->remove(id);
            }
        }

        bool has(int id, int typeId){
            auto *pool = getPool(typeId);
            return (bool)pool && pool->has(id);
        }

    protected:
        std::vector<std::shared_ptr<Pool>> componentPools;
        ComponentPool<SignatureBitMap> entityPool;
        std::map<EntityId, bool> freeEntityIds;
        EntityId nextEntityId;
        int onRegisterCallbackId;
        int onUnregisterCallbackId;

        void setupPool(uint32_t cid){
            if(cid < ComponentRegister::componentPools.size()) {
                if (ComponentRegister::componentPools[cid]) {
                    while (cid >= componentPools.size()) {
                        componentPools.push_back(nullptr);
                    }
                    if (componentPools[cid] == nullptr) {
                        componentPools[cid] = ComponentRegister::componentPools[cid]->make();
                        componentPools[cid]->registry = this;
                        componentPools[cid]->onAdd().add([cid](Registry *reg, EntityId id) {
                            *(SignatureBitMap *) reg->entityPool.getById(id) |= (SignatureBitMap(1) << cid);
                        });
                        componentPools[cid]->onRemove().add([cid](Registry *reg, EntityId id) {
                            *(SignatureBitMap *) reg->entityPool.getById(id) &= ~(SignatureBitMap(1) << cid);
                        });
                    }
                }
            }
        }
    };

}

#include "tridot/ecs/View.h"

#endif //TRIDOT_REGISTRY_H

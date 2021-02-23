//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_REGISTRY_H
#define TRIDOT_REGISTRY_H

#include "TypeMap.h"
#include "Pool.h"
#include "ComponentPool.h"
#include <cstdint>
#include <map>
#include <functional>

namespace ecs {

    class Registry {
    public:
        Registry(){
            nextEntityId = 0;
        }

        EntityId create(EntityId hint = -1){
            EntityId id = -1;
            if(hint != -1){
                if(!entityPool.has(hint)){
                    id = hint;
                }
            }
            if(id == -1){
                if(!freeEntityIds.empty()){
                    id = freeEntityIds.begin()->first;
                    freeEntityIds.erase(freeEntityIds.begin());
                }else{
                    id = nextEntityId++;
                }
            }
            entityPool.add(id, nullptr);
            return id;
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
            entityPool.remove(id);
            for(auto &pool : componentPools){
                if(pool != nullptr){
                    if(pool->has(id)){
                        pool->remove(id);
                    }
                }
            }
            if(id == nextEntityId - 1){
                while(!entityPool.has(nextEntityId - 1)){
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
            auto &pool = assurePool<Component>();
            pool.add(id, std::forward<Args>(args)...);
            return *(Component*)pool.get(id);
        }

        template<typename... Components>
        void add(EntityId id, const Components&... comps){
            (add<Components, const Components &>(id, comps) , ...);
        }

        template<typename Component>
        Component &get(EntityId id){
            uint32_t cid = componentMap.id<Component>();
            ECS_ASSERT(cid < componentPools.size(), "index out of bounds")
            ECS_ASSERT(componentPools[cid] != nullptr, "component pool not present")
            return *(Component*)componentPools[cid]->getById(id);
        }

        template<typename... Components>
        void remove(EntityId id){
            (assurePool<Components>().remove(id) , ...);
        }

        template<typename Component>
        bool has(EntityId id){
            uint32_t cid = componentMap.id<Component>();
            if(cid >= componentPools.size()){
                return false;
            }
            if(componentPools[cid] == nullptr){
                return false;
            }
            return componentPools[cid]->has(id);
        }

        template<typename... Components>
        bool hasAll(EntityId id){
            return (has<Components>(id) && ...);
        }

        template<typename... Components>
        bool hasAny(EntityId id){
            return (has<Components>(id) || ...);
        }

        template<typename... Components, typename Func>
        void each(const Func &func){
            if constexpr (sizeof...(Components) == 0){
                for(EntityId id : entityPool.getEntities()){
                    func(id);
                }
            }else if constexpr (sizeof...(Components) == 1){
                Pool *pool = &assurePool<Components...>();
                for(int i = 0; i < pool->getEntities().size(); i++){
                    func(pool->getId(i), *(Components*)pool->get(i)...);
                }
            }else{
                Pool *pool = &entityPool;

                ((assurePool<Components>().getEntities().size() < pool->getEntities().size() ?
                    pool = &assurePool<Components>() : pool) , ...);

                for(EntityId id : pool->getEntities()){
                    if(hasAll<Components...>(id)){
                        func(id, get<Components>(id)...);
                    }
                }
            }
        }

    private:
        TypeMap componentMap;
        std::vector<std::shared_ptr<Pool>> componentPools;
        Pool entityPool;
        std::map<EntityId, bool> freeEntityIds;
        EntityId nextEntityId;

        template<typename Component>
        ComponentPool<Component> &assurePool(){
            uint32_t cid = componentMap.id<Component>();
            while(cid >= componentPools.size()){
                componentPools.push_back(nullptr);
            }
            if(componentPools[cid] == nullptr){
                componentPools[cid] = std::make_shared<ComponentPool<Component>>();
            }
            return *(ComponentPool<Component>*)componentPools[cid].get();
        }
    };

}

#endif //TRIDOT_REGISTRY_H

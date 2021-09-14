//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "pch.h"
#include "core/core.h"
#include "Scene.h"

namespace tri {

    template<typename... Components>
    class EntityView {
    public:
        EntitySignatureBitmap whitelist;
        EntitySignatureBitmap blacklist;
        Scene* scene;

        EntityView(Scene *scene) {
            this->scene = scene;
            whitelist = scene->createSignature<Components...>();
            blacklist = 0;
        }

        template<typename... Comps>
        EntityView &except() {
            blacklist |= scene->createSignature<Comps...>();
            return *this;
        }

        template<typename Func>
        void each(const Func& func) {
            if constexpr (sizeof...(Components) == 1) {
                //single component iteration
                poolIteration<Components...>(func);
            }
            else if constexpr (sizeof...(Components) == 0) {
                //no component iteration
                poolIteration(func, scene->getEntityPool());
            }
            else {
                //multi component iteration
                ComponentPool* pool = scene->getEntityPool();
                ((pool = (scene->getComponentPool<Components>()->size() <= pool->size()) ? scene->getComponentPool<Components>() : pool), ...);
                bool done = (((pool == scene->getComponentPool<Components>()) ? poolIteration<Components>(func) : false) || ...);
                TRI_ASSERT(done, "no proper component pool found");
            }
        }

    private:

        template<typename IterationComponent, typename Func>
        bool poolIteration(const Func& func) {
            if constexpr (sizeof...(Components) == 1) {
                //single component iteration
                ComponentPool* pool = scene->getComponentPool<IterationComponent>();
                IterationComponent* data = (IterationComponent*)pool->elementData();
                EntityId* idData = pool->idData();
                if (blacklist == 0) {
                    for (EntityId index = 0; index < pool->size(); index++) {
                        if constexpr (std::is_invocable_v<Func, EntityId, Components &...>) {
                            func(idData[index], data[index]);
                        }
                        else {
                            func(data[index]);
                        }
                    }
                }
                else {
                    for (EntityId index = 0; index < pool->size(); index++) {
                        EntityId id = idData[index];
                        if ((scene->getSignature(id) & blacklist) == 0) {
                            if constexpr (std::is_invocable_v<Func, EntityId, Components &...>) {
                                func(id, data[index]);
                            }
                            else {
                                func(data[index]);
                            }
                        }
                    }
                }
            }
            else {
                //multi component iteration
                ComponentPool* pool = scene->getComponentPool<IterationComponent>();
                IterationComponent* data = (IterationComponent*)pool->elementData();
                EntityId* idData = pool->idData();
                for (EntityId index = 0; index < pool->size(); index++) {
                    EntityId id = idData[index];
                    EntitySignatureBitmap signature = scene->getSignature(id);
                    if ((signature & whitelist) == whitelist) {
                        if (blacklist == 0 || (signature & blacklist) == 0) {
                            if constexpr (std::is_invocable_v<Func, EntityId, Components &...>) {
                                func(id,
                                    ((std::is_same_v<IterationComponent, Components>) ?
                                    ((Components&)data[index]) :
                                    (scene->getComponent<Components>(id)))...
                                );
                            }
                            else {
                                func(
                                    ((std::is_same_v<IterationComponent, Components>) ?
                                    ((Components&)data[index]) :
                                    (scene->getComponent<Components>(id)))...
                                );
                            }
                        }
                    }
                }
            }
            return true;
        }

        template<typename Func>
        bool poolIteration(const Func& func, ComponentPool *pool) {
            //no component iteration
            EntityId* idData = pool->idData();
            if (blacklist == 0) {
                for (EntityId index = 0; index < pool->size(); index++) {
                    EntityId id = idData[index];
                    func(id);
                }
            }
            else {
                for (EntityId index = 0; index < pool->size(); index++) {
                    EntityId id = idData[index];
                    EntitySignatureBitmap signature = scene->getSignature(id);
                    if ((signature & blacklist) == 0) {
                        func(id);
                    }
                }
            }
            return true;
        }

    };

}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include "World.h"

namespace tri {

    template<typename... Components>
    class EntityViewNoConst {
    public:
        EntitySignature whitelist;
        EntitySignature blacklist;
        World* world;
        int subviewCount;
        int subviewIndex;
        bool shouldLock[sizeof...(Components) + 1];

        EntityViewNoConst(World* world) {
            this->world = world;
            whitelist = world->createSignature<Components...>();
            blacklist = 0;
            for (int i = 0; i < sizeof...(Components) + 1; i++) {
                shouldLock[i] = true;
            }
            subviewCount = 1;
            subviewIndex = 0;
        }

        template<typename... Comps>
        EntityViewNoConst& except() {
            blacklist |= world->createSignature<Comps...>();
            return *this;
        }

        EntityViewNoConst& subview(int index, int subviewCount) {
            this->subviewCount = subviewCount;
            this->subviewIndex = index;
            return *this;
        }

        template<typename Func>
        void each(const Func& func) {
            int storageCount = 0;
            ComponentStorage* storages[sizeof...(Components) + 1];
            ((storages[storageCount++] = world->getComponentStorage<Components>()), ...);

            for (int i = 0; i < storageCount; i++) {
                if (!storages[i]) {
                    //if a storage is not present, there are no entities to iterate
                    return;
                }
            }

            for (int i = 0; i < storageCount; i++) {
                if (shouldLock[i]) {
                    storages[i]->lock();
                }
            }

            if constexpr (sizeof...(Components) == 0) {
                entityIteration(func);
            }
            else if constexpr (sizeof...(Components) == 1) {
                singleComponentIteration<Components...>(func);
            }
            else {
                //multi component iteration
                ComponentStorage* storage = world->getEntityStorage();
                ((storage = (world->getComponentStorage<Components>()->size() <= storage->size()) ? world->getComponentStorage<Components>() : storage), ...);
                bool done = (((storage == world->getComponentStorage<Components>()) ? multiComponentiteration<Components>(func) : false) || ...);
                TRI_ASSERT(done, "no proper component storage found");
            }

            for (int i = 0; i < storageCount; i++) {
                if (shouldLock[i]) {
                    storages[i]->unlock();
                }
            }
        }

        template<typename Func>
        void multithreadedEach(int taskCount, const Func& func) {
            int storageCount = 0;
            ComponentStorage* storages[sizeof...(Components) + 1];
            ((storages[storageCount++] = world->getComponentStorage<Components>()), ...);
            for (int i = 0; i < storageCount; i++) {
                if (!storages[i]) {
                    //if a storage is not present, there are no entities to iterate
                    return;
                }
                if (shouldLock[i]) {
                    storages[i]->lock();
                }
            }

            std::vector<int> tasks;
            for (int i = 0; i < taskCount; i++) {
                tasks.push_back(env->threadManager->addTask([this, func, i, taskCount]() {
                    EntityViewNoConst view(*this);
                    for (int i = 0; i < sizeof...(Components); i++) {
                        view.shouldLock[i] = false;
                    }
                    view.subview(i, taskCount).each(func);
                }));
            }
            for (int task : tasks) {
                env->threadManager->joinTask(task);
            }

            for (int i = 0; i < storageCount; i++) {
                if (shouldLock[i]) {
                    storages[i]->unlock();
                }
            }
        }

    private:

        template<typename IterationComponent, typename Func>
        bool multiComponentiteration(const Func& func) {
            ComponentStorage* storage = world->getComponentStorage<IterationComponent>();
            EntityId* idData = storage->getIdData();

            int groupSize = storage->getGroupSize<Components...>();

            if (groupSize != -1 && blacklist == 0) {
                //aligned storages iteration
                EntityId start = (groupSize / subviewCount) * subviewIndex;
                EntityId end = (groupSize / subviewCount) * (subviewIndex + 1);
                if (subviewIndex == subviewCount - 1) {
                    end = groupSize;
                }

                int dataIndex = sizeof...(Components);
                uint8_t* datas[sizeof...(Components)];
                ((datas[--dataIndex] = (uint8_t*)world->getComponentStorage<Components>()->getComponentData()), ...);

                for (EntityId index = start; index < end; index++) {
                    if constexpr (std::is_invocable_v<Func, EntityId, Components &...>) {
                        int dataIndex = -1;
                        func(idData[index],
                            (((Components*)datas[++dataIndex])[index])...
                        );
                    }
                    else {
                        int dataIndex = -1;
                        func(
                            (((Components*)datas[++dataIndex])[index])...
                        );
                    }
                }
            }
            else {
                //unaligned storages iteration
                IterationComponent* data = (IterationComponent*)storage->getComponentData();
                
                EntityId start = (storage->size() / subviewCount) * subviewIndex;
                EntityId end = (storage->size() / subviewCount) * (subviewIndex + 1);
                if (subviewIndex == subviewCount - 1) {
                    end = storage->size();
                }

                int storageIndex = sizeof...(Components);
                ComponentStorage* storages[sizeof...(Components)];
                ((storages[--storageIndex] = world->getComponentStorage<Components>()), ...);

                for (EntityId index = start; index < end; index++) {
                    EntityId id = idData[index];
                    EntitySignature signature = world->getSignature(id);
                    if ((signature & whitelist) == whitelist) {
                        if (blacklist == 0 || (signature & blacklist) == 0) {
                            if constexpr (std::is_invocable_v<Func, EntityId, Components &...>) {
                                int storageIndex = -1;
                                func(id,
                                    ((std::is_same_v<IterationComponent, Components>) ?
                                        (++storageIndex, (Components&)data[index]) :
                                        (*(Components*)storages[++storageIndex]->getComponentByIdUnchecked(id)))...
                                );
                            }
                            else {
                                int storageIndex = -1;
                                func(
                                    ((std::is_same_v<IterationComponent, Components>) ?
                                        (++storageIndex, (Components&)data[index]) :
                                        (*(Components*)storages[++storageIndex]->getComponentByIdUnchecked(id)))...
                                );
                            }
                        }
                    }
                }
            }
            return true;
        }

        template<typename IterationComponent, typename Func>
        bool singleComponentIteration(const Func& func) {
            ComponentStorage* storage = world->getComponentStorage<IterationComponent>();
            IterationComponent* data = (IterationComponent*)storage->getComponentData();
            EntityId* idData = storage->getIdData();

            EntityId start = (storage->size() / subviewCount) * subviewIndex;
            EntityId end = (storage->size() / subviewCount) * (subviewIndex + 1);
            if (subviewIndex == subviewCount - 1) {
                end = storage->size();
            }

            if (blacklist == 0) {
                for (EntityId index = start; index < end; index++) {
                    if constexpr (std::is_invocable_v<Func, EntityId, Components &...>) {
                        func(idData[index], data[index]);
                    }
                    else {
                        func(data[index]);
                    }
                }
            }
            else {
                for (EntityId index = start; index < end; index++) {
                    EntityId id = idData[index];
                    if ((world->getSignature(id) & blacklist) == 0) {
                        if constexpr (std::is_invocable_v<Func, EntityId, Components &...>) {
                            func(id, data[index]);
                        }
                        else {
                            func(data[index]);
                        }
                    }
                }
            }

            return true;
        }

        template<typename Func>
        bool entityIteration(const Func& func) {
            ComponentStorage *storage = world->getEntityStorage();
            EntityId* idData = storage->getIdData();

            EntityId start = (storage->size() / subviewCount) * subviewIndex;
            EntityId end = (storage->size() / subviewCount) * (subviewIndex + 1);
            if (subviewIndex == subviewCount - 1) {
                end = storage->size();
            }

            if (blacklist == 0) {
                for (EntityId index = start; index < end; index++) {
                    func(idData[index]);
                }
            }
            else {
                for (EntityId index = start; index < end; index++) {
                    EntityId id = idData[index];
                    if ((world->getSignature(id) & blacklist) == 0) {
                        func(id);
                    }
                }
            }
            return true;
        }

    };


    template<typename... Components>
    class EntityView : public EntityViewNoConst<std::remove_const_t<Components>...> {
    public:
        typedef EntityViewNoConst<std::remove_const_t<Components>...> Super;

        EntityView(World* world) : Super(world) {
            int index = 0;
            ((Super::shouldLock[index++] = (std::is_same_v<Components, std::remove_const_t<Components>>)), ...);
        }
    };

}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "pch.h"
#include "core/core.h"
#include "ComponentPool.h"
#include "ComponentBuffer.h"
#include "engine/Asset.h"

namespace tri {

    template<typename... Components>
    class EntityView;

    class Scene : public Asset, public System {
    public:
        std::string file;

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
        void clearComponent(int typeId);
        void copy(const Scene &from);
        void swap(Scene &scene);
        void update() override;
        void enablePendingOperations(bool enable);

        template<typename Component>
        ComponentPool* getComponentPool() {
            return getComponentPool(env->reflection->getTypeId<Component>());
        }

        template<typename... Components>
        EntitySignatureBitmap createSignature() {
            EntitySignatureBitmap signature = 0;
            ((signature |= ((EntitySignatureBitmap)1 << env->reflection->getTypeId<Components>())) , ...);
            return signature;
        }

        template<typename Component>
        EntityId getEntityIdByComponent(const Component &comp) {
            auto* pool = getComponentPool<Component>();
            EntityId index = &comp - (Component*)pool->elementData();
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

        bool load(const std::string &file) override;
        bool save(const std::string &file) override;
        static void loadMainScene(const std::string &file);

    private:
        std::vector<std::shared_ptr<ComponentPool>> pools;
        ComponentPool entityPool;
        std::unordered_set<EntityId> freeList;

        class PendingOperation {
        public:
            EntityId id;
            int typeId;
            bool isAddOperation;
            bool isPending;
            ComponentBuffer component;
        };
        bool pendingOperationsEnabled;
        bool addPendingOperations;
        std::vector<PendingOperation> pendingOperations;
    };

}

#include "EntityView.h"

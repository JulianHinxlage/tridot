//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "pch.h"
#include "core/core.h"
#include "Scene.h"

namespace tri {

    class Prefab : public Asset {
    public:
        Prefab();
        Prefab(const Prefab& prefab);

        template<typename... Components>
        Prefab(const Components&... comps) {
            addComponents(comps...);
        }

        template<typename... Components>
        void addComponents(const Components&... comps) {
            ((addComponent<Components>(comps)), ...);
        }

        template<typename Component, typename... Args>
        Component& addComponent(const Args&... args) {
            Component *comp = (Component*)addComponent(env->reflection->getTypeId<Component>());
            new (comp) Component(args...);
            return *comp;
        }

        template<typename Component>
        Component& getComponent() {
            return *(Component*)getComponent(env->reflection->getTypeId<Component>());
        }

        template<typename Component>
        bool hasComponent() {
            return hasComponent(env->reflection->getTypeId<Component>());
        }

        template<typename Component>
        bool removeComponent() {
            return removeComponent(env->reflection->getTypeId<Component>());
        }

        void *addComponent(int typeId);
        void *getComponent(int typeId);
        bool hasComponent(int typeId);
        bool removeComponent(int typeId);

        template<typename... Components>
        Prefab& addChild(const Components&... comps) {
            children.push_back(std::make_shared<Prefab>(comps...));
            return *children.back().get();
        }

        EntityId createEntity(Scene* scene, EntityId hint = -1);
        void copyEntity(EntityId id, Scene *scene, bool copyChildren = true);
        void operator=(const Prefab& prefab);
        void clear();
        std::vector<ComponentBuffer> &getComponents(){return comps;}
        std::vector<std::shared_ptr<Prefab>> &getChildren(){return children;}

        bool load(const std::string &file) override;
        bool save(const std::string &file) override;

    private:
        std::vector<ComponentBuffer> comps;
        std::vector<std::shared_ptr<Prefab>> children;
    };

}

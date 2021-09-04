//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/entity/Registry.h"
#include "tridot/components/Transform.h"

namespace tridot {

    class Scene : public Registry{
    public:
        std::string name;
        std::string file;

        bool load();
        bool save();
        bool load(const std::string &file);
        bool save(const std::string &file);
        bool preLoad(const std::string &file);
        bool postLoad();

        void copy(const Scene &source);
        void swap(Scene &other);

        template<typename Component>
        Component *getComponentByHierarchy(EntityId id){
            if(has<Component>(id)){
                return &get<Component>(id);
            }
            if(has<Transform>(id)){
                EntityId parent = get<Transform>(id).parent.id;
                if(parent != -1){
                    return getComponentByHierarchy<Component>(parent);
                }
            }
            return nullptr;
        }

        template<typename Component>
        EntityId getIdByComponent(Component &component){
            ComponentPool<Component> &pool = getPool<Component>();
            if(pool.getEntities().size() > 0){
                int index = &component - (Component*)pool.get(0);
                if(index >= 0 && index < pool.getEntities().size()){
                    return pool.getId(index);
                }
            }
            return -1;
        }
    };

}


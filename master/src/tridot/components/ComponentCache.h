//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/core/Environment.h"
#include "tridot/core/config.h"
#include <yaml-cpp/yaml.h>

namespace tridot {

    //this cache holds data of unknown components of an entity
    //when an entity has a component which is implemented in a plugin the component can not be added to the ecs
    //in this case the data is cached
    //when the plugin is loaded at a later point the component can be reconstructed
    //also the data is preserved for subsequent serialisation operations
    class ComponentCache {
    public:
        YAML::Node data;

        bool isCached(int typeId);
        bool load(int typeId, void *ptr);
        void remove(int typeId);
        void update(EntityId id);

        template<typename T>
        bool isCached(){
            return isCahced(env->reflection->getTypeId<T>());
        }
        template<typename T>
        bool load(T &t){
            return load(env->reflection->getTypeId<T>(), &t);
        }
        template<typename T>
        void remove(){
            remove(env->reflection->getTypeId<T>());
        }
    };

}


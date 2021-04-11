//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_COMPONENTCACHE_H
#define TRIDOT_COMPONENTCACHE_H

#include "tridot/ecs/Reflection.h"
#include "tridot/ecs/config.h"
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

        bool isCached(int reflectId);
        bool load(int reflectId, void *ptr);
        void remove(int reflectId);
        void update(ecs::EntityId id);

        template<typename T>
        bool isCached(){
            return isCahced(ecs::Reflection::id<T>());
        }
        template<typename T>
        bool load(T &t){
            return load(ecs::Reflection::id<T>(), &t);
        }
        template<typename T>
        void remove(){
            remove(ecs::Reflection::id<T>());
        }
    };

}

#endif //TRIDOT_COMPONENTCACHE_H

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_COMPONENTCACHE_H
#define TRIDOT_COMPONENTCACHE_H

#include "tridot/ecs/Reflection.h"
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
            return isCahced(Reflection::id<T>());
        }
        template<typename T>
        bool load(T &t){
            return load(Reflection::id<T>(), &t);
        }
        template<typename T>
        void remove(){
            remove(Reflection::id<T>());
        }
    };

}

#endif //TRIDOT_COMPONENTCACHE_H

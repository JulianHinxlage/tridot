//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "ComponentPool.h"
#include "TypeMap.h"

namespace tridot {

    class ComponentRegister {
    public:
        template<typename... Components>
        static void registerComponent(){
            (registerPool<Components>() , ...);
        }

        template<typename... Components>
        static void unregisterComponent(){
            (unregisterComponent(env->reflection->getTypeId<Components>()) , ...);
        }

        static void unregisterComponent(int typeId){
            uint32_t cid = componentMap.id(typeId);
            if(cid < componentPools.size()){
                env->events->componentUnregister.invoke(typeId);
                componentPools[cid] = nullptr;
            }
        }

        template<typename Component>
        static int id(){
            return componentMap.id<Component>();
        }

        static int id(int typeId){
            return componentMap.id(typeId);
        }

    private:
        friend class Registry;
        static TypeMap componentMap;
        static std::vector<std::shared_ptr<Pool>> componentPools;

        template<typename Component>
        static void registerPool(){
            uint32_t cid = componentMap.id<Component>();
            while(cid >= componentPools.size()){
                componentPools.push_back(nullptr);
            }
            if(componentPools[cid] == nullptr){
                componentPools[cid] = std::make_shared<ComponentPool<Component>>();
                env->events->componentRegister.invoke(env->reflection->getTypeId<Component>());
            }
        }

    };

    namespace impl {
        template<typename T>
        class ComponentRegisterer{
        public:
            ComponentRegisterer(){
                tridot::ComponentRegister::registerComponent<T>();
            }
            ~ComponentRegisterer(){
                tridot::ComponentRegister::unregisterComponent<T>();
            }
        };
    }

}

#define TRI_REGISTER_COMPONENT(type) tridot::impl::ComponentRegisterer<type> TRI_UNIQUE_IDENTIFIER(_tri_register_component);

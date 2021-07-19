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
                //onUnregisterSignal.invoke(typeId);
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

        //static auto onRegister(){
        //    return onRegisterSignal.ref();
        //}

        //static auto onUnregister(){
        //    return onUnregisterSignal.ref();
        //}

    private:
        friend class Registry;
        static TypeMap componentMap;
        static std::vector<std::shared_ptr<Pool>> componentPools;
        //static Signal<int> onRegisterSignal;
        //static Signal<int> onUnregisterSignal;

        template<typename Component>
        static void registerPool(){
            uint32_t cid = componentMap.id<Component>();
            while(cid >= componentPools.size()){
                componentPools.push_back(nullptr);
            }
            if(componentPools[cid] == nullptr){
                componentPools[cid] = std::make_shared<ComponentPool<Component>>();
                env->events->componentRegister.invoke(env->reflection->getTypeId<Component>());
                //onRegisterSignal.invoke(env->reflection->getTypeId<Component>());
            }
        }

    };

}

#define TRI_REGISTER_COMPONENT(type) TRI_REGISTER_CALLBACK(){ tridot::ComponentRegister::registerComponent<type>(); }

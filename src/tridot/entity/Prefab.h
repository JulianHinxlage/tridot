//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_PREFAB_H
#define TRIDOT_PREFAB_H

#include "Registry.h"

namespace tridot {

    class Prefab {
    public:
        template<typename... Components>
        Prefab(const Components&... comps){
            add(comps...);
        }

        EntityId instantiateHinted(Registry &reg, EntityId hint = -1){
            EntityId id = reg.createHinted(hint);
            for(auto &component : components){
                if(component != nullptr){
                    component->add(reg, id);
                }
            }
            return id;
        }

        template<typename... Components>
        EntityId instantiate(Registry &reg, const Components&... comps){
            EntityId id = instantiateHinted(reg);
            reg.add(id, comps...);
            return id;
        }

        template<typename Component, typename... Args>
        Component &add(Args&&... args){
            uint32_t id = map.id<Component>();
            while(id >= components.size()){
                components.push_back(nullptr);
            }
            if(components[id] == nullptr){
                components[id] = std::make_shared<ComponentT<Component>>(std::forward<Args>(args)...);
            }
            return (*(ComponentT<Component>*)components[id].get()).component;
        }

        template<typename... Components>
        void add(const Components&... comps){
            (add<Components, const Components&>(comps) , ...);
        }

        template<typename... Components>
        void remove(){
            ((map.id<Components>() < components.size() && (components[map.id<Components>()] = nullptr)) , ...);
        }

        template<typename Component>
        Component &get(){
            uint32_t id = map.id<Component>();
            if(id >= components.size()){
                return *(Component*)nullptr;
            }
            return (*(ComponentT<Component>*)components[id].get()).component;
        }

        template<typename Component>
        bool has(){
            uint32_t id = map.id<Component>();
            if(id >= components.size()){
                return false;
            }
            return components[id] != nullptr;
        }

    private:
        class Component {
        public:
            virtual void add(Registry &reg, EntityId id) = 0;
        };

        std::vector<std::shared_ptr<Component>> components;
        TypeMap map;

        template<typename T>
        class ComponentT : public Component{
        public:
            T component;

            template<typename... Args>
            ComponentT(Args&&... args) : component(std::forward<Args>(args)...){}

            virtual void add(Registry &reg, EntityId id) override {
                reg.add<T>(id, component);
            }
        };
    };

}

#endif //TRIDOT_PREFAB_H

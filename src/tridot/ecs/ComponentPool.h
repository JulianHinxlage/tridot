//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_COMPONENTPOOL_H
#define TRIDOT_COMPONENTPOOL_H

#include "Pool.h"

namespace tridot {

    template<typename Component>
    class ComponentPool : public Pool {
    public:
        virtual void* get(uint32_t index) override {
            TRI_ASSERT(index >= 0 && index <= components.size(), "index out of bounds")
            return &components[index];
        }

        virtual uint32_t add(EntityId id, void *instance) override {
            uint32_t index = Pool::add(id, instance);
            if(index == components.size()){
                if(instance == nullptr){
                    components.emplace_back();
                }else{
                    components.emplace_back(*(Component*)instance);
                }
            }else{
                if(instance == nullptr){
                    components[index] = Component();
                }else{
                    components[index] = *(Component*)instance;
                }
            }
            return index;
        }

        template<typename... Args>
        uint32_t add(EntityId id, Args&&... args){
            uint32_t index = Pool::add(id, nullptr);
            if(index == components.size()){
                components.emplace_back(std::forward<Args>(args)...);
            }else{
                components[index] = Component(args...);
            }
            return index;
        }

        virtual uint32_t remove(EntityId id) override {
            uint32_t index = Pool::remove(id);
            if(index != -1){
                components[index] = components.back();
                components.pop_back();
            }
            return index;
        }

        virtual void swap(uint32_t index1, uint32_t index2) override {
            Pool::swap(index1, index2);
            std::swap(components[index1], components[index2]);
        }

        virtual void clear() override{
            Pool::clear();
            components.clear();
        }

        virtual void copy(const Pool &source) override{
            Pool::copy(source);
            components = ((ComponentPool<Component>&)source).components;
        }

        virtual std::shared_ptr<Pool> make() override{
            return std::make_shared<ComponentPool<Component>>();
        }

    private:
        std::vector<Component> components;
    };

}

#endif //TRIDOT_COMPONENTPOOL_H

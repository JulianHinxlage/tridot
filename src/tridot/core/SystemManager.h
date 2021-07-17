//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_SYSTEMMANAGER_H
#define TRIDOT_SYSTEMMANAGER_H

#include <unordered_map>
#include <memory>

namespace tridot {

    class SystemManager{
    public:
        template<typename System>
        System *getSystem(){
            size_t type = typeid(System).hash_code();
            if(systems.contains(type)){
                return (System*)systems[type].get();
            }else{
                return nullptr;
            }
        }

        template<typename System, typename... Args>
        System *addSystem(Args&& ...args){
            size_t type = typeid(System).hash_code();
            systems[type] = std::shared_ptr<void>(new System(std::forward<Args>(args)...), [](void *ptr){delete (System*)ptr;});
            return (System*)systems[type].get();
        }

        template<typename System>
        bool removeSystem(){
            size_t type = typeid(System).hash_code();
            if(systems.contains(type)){
                systems.erase(type);
                return true;
            }else{
                return false;
            }
        }

        template<typename System>
        System *setSystem(System *system){
            size_t type = typeid(System).hash_code();
            systems[type] = std::shared_ptr<void>(system, [](void *ptr){});
            return (System*)systems[type].get();
        }

        template<typename System>
        bool hasSystem(){
            return getSystem<System>() != nullptr;
        }

        void clearSystems(){
            systems.clear();
        }

    private:
        std::unordered_map<size_t, std::shared_ptr<void>> systems;
    };

}

#endif //TRIDOT_SYSTEMMANAGER_H

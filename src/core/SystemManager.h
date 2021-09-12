//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "pch.h"
#include "System.h"
#include "Environment.h"
#include "SignalManager.h"

namespace tri {

    class SystemManager{
    public:
        SystemManager() {
            startupFlag = false;
        }

        ~SystemManager() {
            shutdown();
        }

        template<typename T>
        T *getSystem(){
            size_t type = typeid(T).hash_code();
            if(systems.contains(type)){
                return (T*)systems[type].system.get();
            }else{
                return nullptr;
            }
        }

        template<typename T, typename... Args>
        T *addSystem(const std::string &name, Args&& ...args){
            size_t type = typeid(T).hash_code();
            return (T*)setupSystem(std::shared_ptr<System>(new T(std::forward<Args>(args)...), [](System* ptr) {delete (T*)ptr; }), name, type);
        }

        template<typename T>
        bool removeSystem(){
            size_t type = typeid(T).hash_code();
            if(systems.contains(type)){
                if (systems[type].system) {
                    systems[type].system->shutdown();
                    env->signals->update.removeCallback(systems[type].updateCallbackId);
                }
                systems.erase(type);
                return true;
            }else{
                return false;
            }
        }

        template<typename T>
        T *setSystem(const std::string& name, T *system){
            size_t type = typeid(T).hash_code();
            return (T*)setupSystem(std::shared_ptr<System>(system, [](System* ptr) {}), name, type);
        }

        template<typename T>
        bool hasSystem(){
            return getSystem<T>() != nullptr;
        }

        void startup() {
            startupFlag = true;
            for (auto sys : systems) {
                if (sys.second.system) {
                    if (sys.second.startupFlag == false) {
                        sys.second.startupFlag = true;
                        sys.second.system->startup();
                    }
                }
            }
        }

        void shutdown(){
            for (auto sys : systems) {
                if (sys.second.system) {
                    sys.second.system->shutdown();
                    env->signals->update.removeCallback(sys.second.updateCallbackId);
                    sys.second.system = nullptr;
                }
            }
            systems.clear();
        }

    private:
        System* setupSystem(std::shared_ptr<System> system, const std::string& name, size_t type) {
            SystemRecord& r = systems[type];
            r.system = system;
            r.startupFlag = false;
            r.name = name;
            r.updateCallbackId = env->signals->update.addCallback(name, [system = r.system.get()]() { system->update(); });
            if (startupFlag) {
                r.startupFlag = true;
                r.system->startup();
            }
            return r.system.get();
        }

        class SystemRecord {
        public:
            std::shared_ptr<System> system;
            bool startupFlag;
            std::string name;
            int updateCallbackId;
        };

        std::unordered_map<size_t, SystemRecord> systems;
        bool startupFlag;
    };

}


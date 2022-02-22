//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "System.h"
#include "Reflection.h"
#include "Module.h"
#include "Profiler.h"
#include "Signal.h"

namespace tri {

    class Scene;

    class SignalManager : public System{
    public:
        Signal<> preStartup;
        Signal<> startup;
        Signal<> postStartup;
        Signal<> preUpdate;
        Signal<> update;
        Signal<> postUpdate;
        Signal<> preShutdown;
        Signal<> shutdown;
        Signal<> postShutdown;
        Signal<Scene*> sceneLoad;
        Signal<Scene*> sceneBegin;
        Signal<Scene*> sceneEnd;
        Signal<> runtimeModeChanged;
        Signal<Module*> moduleLoad;
        Signal<Module*> moduleUnload;
        Signal<int> typeRegister;
        Signal<int> typeUnregister;
        Signal<EntityId, Scene*> entityCreate;
        Signal<EntityId, Scene*> entityRemove;

        SignalManager(){
            setSignal(&preStartup, "preStartup");
            setSignal(&startup, "startup");
            setSignal(&postStartup, "postStartup");
            setSignal(&preUpdate, "preUpdate");
            setSignal(&update, "update");
            setSignal(&postUpdate, "postUpdate");
            setSignal(&preShutdown, "preShutdown");
            setSignal(&shutdown, "shutdown");
            setSignal(&postShutdown, "postShutdown");
            setSignal(&sceneLoad, "sceneLoad");
            setSignal(&sceneBegin, "sceneBegin");
            setSignal(&sceneEnd, "sceneEnd");
            setSignal(&moduleLoad, "moduleLoad");
            setSignal(&moduleUnload, "moduleUnload");
            setSignal(&typeRegister, "typeRegister");
            setSignal(&typeUnregister, "typeUnregister");
            setSignal(&entityCreate, "entityCreate");
            setSignal(&entityRemove, "entityRemove");
        }

        template<typename... Args>
        Signal<Args...> &getSignal(const std::string &name = ""){
            size_t type = typeid(Signal<Args...>).hash_code();
            if((signals.find(type) == signals.end()) || (signals[type].find(name) == signals[type].end())) {
                signals[type][name] = std::make_shared<Signal<Args...>>();
            }
            return *(Signal<Args...>*)signals[type][name].get();
        }

        template<typename... Args>
        void setSignal(Signal<Args...> *signal, const std::string &name = ""){
            size_t type = typeid(Signal<Args...>).hash_code();
            signals[type][name] = std::shared_ptr<Signal<Args...>>(signal, [](Signal<Args...> *ptr){});
        }

        void pollSignals(){
            for(auto &typeSignals : signals){
                for(auto &signal : typeSignals.second){
                    if(signal.second){
                        signal.second->pollSignals();
                    }
                }
            }
        }

        template<typename Component>
        Signal<EntityId, Scene*>& getComponentInit() {
            return getComponentInit(env->reflection->getTypeId<Component>());
        }

        template<typename Component>
        Signal<EntityId, Scene*>& getComponentShutdown() {
            return getComponentShutdown(env->reflection->getTypeId<Component>());
        }

        Signal<EntityId, Scene*>& getComponentInit(int typeId) {
            if (typeId >= componentInitSignals.size()) {
                componentInitSignals.resize(typeId + 1);
            }
            return componentInitSignals[typeId];
        }

        Signal<EntityId, Scene*>& getComponentShutdown(int typeId) {
            if (typeId >= componentShutdownSignals.size()) {
                componentShutdownSignals.resize(typeId + 1);
            }
            return componentShutdownSignals[typeId];
        }

    private:
        std::unordered_map<size_t, std::unordered_map<std::string, std::shared_ptr<BaseSignal>>> signals;
        std::vector<Signal<EntityId, Scene*>> componentInitSignals;
        std::vector<Signal<EntityId, Scene*>> componentShutdownSignals;
    };

}


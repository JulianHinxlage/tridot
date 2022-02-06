//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "System.h"
#include "Reflection.h"
#include "Module.h"
#include "Profiler.h"

namespace tri {

    class BaseSignal{
    public:
        virtual void pollSignals() = 0;
    };

    template<typename... Args>
    class Signal : public BaseSignal{
    public:
        typedef std::function<void(Args...)> Callback;
        class Observer{
        public:
            Callback callback;
            std::string name;
            int id;
            bool active;
            bool swapped;
        };

        Signal(){
            nextId = 0;
            handledFlag = false;
        }

        int addCallback(const std::string &name, const Callback &callback){
            return addCallback(callback, name);
        }

        int addCallback(const Callback &callback, const std::string &name = ""){
            int id = nextId++;
            int index = checkDependencies(name);
            observers.insert(observers.begin() + index, {callback, name, id, true, false});
            for (auto& onAdd : addCallbacks) {
                if (onAdd) {
                    onAdd(name);
                }
            }
            return id;
        }

        void removeCallback(int id){
            for(int i = 0; i < observers.size(); i++){
                if(observers[i].id == id){
                    observers.erase(observers.begin() + i);
                    i--;
                }
            }
        }

        void removeCallback(const std::string &name){
            for(int i = 0; i < observers.size(); i++){
                if(observers[i].name == name){
                    observers.erase(observers.begin() + i);
                    i--;
                }
            }
        }

        void setActiveCallback(const std::string &callback, bool active = true) {
            for (auto& observer : observers) {
                if (observer.name == callback) {
                    observer.active = active;
                }
            }
        }

        void setActiveCallbacks(const std::vector<std::string> &callbacks, bool active = true){
            for(auto &callback : callbacks){
                setActiveCallback(callback, active);
            }
        }

        void setActiveAll(bool active = true){
            for(auto &observer : observers){
                observer.active = active;
            }
        }

        void clearCallbacks(){
            observers.clear();
        }

        void invoke(Args... args){
            handledFlag = false;
            for(int i = 0; i < observers.size(); i++){
                auto &observer = observers[i];
                if(observer.callback){
                    if(observer.active){
                        observer.callback(args...);
                    }
                }
                if(handledFlag){
                    break;
                }
            }
        }

        void invokeProfile(Args... args) {
            handledFlag = false;
            for (int i = 0; i < observers.size(); i++) {
                auto& observer = observers[i];
                if (observer.callback) {
                    if (observer.active) {
                        TRI_PROFILE_NAME(observer.name.c_str(), observer.name.size());
                        observer.callback(args...);
                    }
                }
                if (handledFlag) {
                    break;
                }
            }
        }

        void invokeAsync(Args... args){
            eventBuffer.push_back([this, args...](){
                invoke(args...);
            });
        }

        virtual void pollSignals() override{
            for(auto &event : eventBuffer){
                event();
            }
            eventBuffer.clear();
        }

        void handled(){
            handledFlag = true;
        }

        void callbackOrder(const std::vector<std::string> &callbacks){
            for(int i = 1; i < callbacks.size(); i++){
                dependencies[callbacks[i]].push_back(callbacks[i-1]);
            }

            int iterationCount = 0;
            bool anySwaps = true;
            while(anySwaps && iterationCount < 100) {
                anySwaps = false;
                iterationCount++;
                for (auto &observer : observers) {
                    observer.swapped = false;
                }
                for (int i = observers.size() - 1; i >= 0; i--) {
                    if (!observers[i].swapped) {
                        int index = checkDependencies(observers[i].name);
                        if (index < i) {
                            Observer tmp = observers[i];
                            tmp.swapped = true;
                            observers.erase(observers.begin() + i);
                            observers.insert(observers.begin() + index, tmp);
                            anySwaps = true;
                            i++;
                        }
                    }
                }
            }
        }

        const std::vector<Observer> &getObservers(){
            return observers;
        }

        void swapObserverPositions(int index1, int index2) {
            if (index1 >= 0 && index1 < observers.size()) {
                if (index2 >= 0 && index2 < observers.size()) {
                    std::swap(observers[index1], observers[index2]);
                }
            }
        }

        void onAddCallback(const std::function<void(const std::string&)>& callback) {
            addCallbacks.push_back(callback);
        }

    private:
        std::vector<Observer> observers;
        std::vector<std::function<void()>> eventBuffer;
        std::unordered_map<std::string, std::vector<std::string>> dependencies;
        int nextId;
        bool handledFlag;
        std::vector<std::function<void(const std::string&)>> addCallbacks;

        //returns the index where an observer should be based on the defined dependencies
        int checkDependencies(const std::string &name){
            for(int i = 0; i < observers.size(); i++) {
                for(auto &dependency : dependencies[observers[i].name]) {
                    if (name == dependency) {
                        return i;
                    }
                }
            }
            return (int)observers.size();
        }
    };

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


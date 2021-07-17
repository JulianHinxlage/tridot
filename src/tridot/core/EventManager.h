//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_EVENTMANAGER_H
#define TRIDOT_EVENTMANAGER_H

#include "config.h"
#include <functional>
#include <memory>

namespace tridot {

    class BaseEventSignal{
    public:
        virtual void pollEvents() = 0;
    };

    template<typename... Args>
    class EventSignal : public BaseEventSignal{
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

        EventSignal(){
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
            setActiveCallbacks({callback}, active);
        }

        void setActiveCallbacks(const std::vector<std::string> &callbacks, bool active = true){
            for(auto &callback : callbacks){
                for(auto &observer : observers){
                    if(observer.name == callback){
                        observer.active = active;
                    }
                }
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
            for(auto &observer : observers){
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

        void invokeAsync(Args... args){
            eventBuffer.push_back([this, args...](){
                invoke(args...);
            });
        }

        virtual void pollEvents() override{
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

            bool anySwaps = true;
            while(anySwaps) {
                anySwaps = false;
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

    private:
        std::vector<Observer> observers;
        std::vector<std::function<void()>> eventBuffer;
        std::unordered_map<std::string, std::vector<std::string>> dependencies;
        int nextId;
        bool handledFlag;

        //returns the index where an observer should be based on the defined dependencies
        int checkDependencies(const std::string &name){
            int index = observers.size();
            for(int i = 0; i < observers.size(); i++) {
                for(auto &dependency : dependencies[observers[i].name]) {
                    if (name == dependency) {
                        if (i < index) {
                            index = i;
                        }
                    }
                }
            }
            return index;
        }
    };

    class EventManager{
    public:
        EventSignal<> init;
        EventSignal<> update;
        EventSignal<> exit;
        EventSignal<> shutdown;

        EventSignal<EntityId> entityCreate;
        EventSignal<EntityId> entityDestroy;

        EventSignal<int> keyPressed;
        EventSignal<int> keyReleased;
        EventSignal<int> keyDown;
        EventSignal<int> mousePressed;
        EventSignal<int> mouseReleased;
        EventSignal<int> mouseDown;
        EventSignal<float, float> mouseMoved;
        EventSignal<int> mouseWheel;

        EventSignal<int, int> windowResize;
        EventSignal<int, int> windowMoved;

        EventManager(){
            setSignal(&init, "init");
            setSignal(&update, "update");
            setSignal(&exit, "exit");
            setSignal(&shutdown, "shutdown");

            setSignal(&entityCreate, "entityCreate");
            setSignal(&entityDestroy, "entityDestroy");

            setSignal(&keyPressed, "keyPressed");
            setSignal(&keyReleased, "keyReleased");
            setSignal(&keyDown, "keyDown");
            setSignal(&mousePressed, "mousePressed");
            setSignal(&mouseReleased, "mouseReleased");
            setSignal(&mouseDown, "mouseDown");
            setSignal(&mouseMoved, "mouseMoved");
            setSignal(&mouseWheel, "mouseWheel");

            setSignal(&windowResize, "windowResize");
            setSignal(&windowMoved, "windowMoved");
        }

        template<typename Component>
        EventSignal<EntityId, Component&> &componentAdd(){
            return getSignal<EntityId, Component&>("componentAdd");
        }

        template<typename Component>
        EventSignal<EntityId, Component&> &componentRemove(){
            return getSignal<EntityId, Component&>("componentRemove");
        }

        template<typename... Args>
        EventSignal<Args...> &getSignal(const std::string &name = ""){
            size_t type = typeid(EventSignal<Args...>).hash_code();
            if(!signals.contains(type) || !signals[type].contains(name)){
                signals[type][name] = std::make_shared<EventSignal<Args...>>();
            }
            return *(EventSignal<Args...>*)signals[type][name].get();
        }

        template<typename... Args>
        void setSignal(EventSignal<Args...> *signal, const std::string &name = ""){
            size_t type = typeid(EventSignal<Args...>).hash_code();
            signals[type][name] = std::shared_ptr<EventSignal<Args...>>(signal, [](EventSignal<Args...> *ptr){});
        }

        void pollEvents(){
            for(auto &typeSignals : signals){
                for(auto &signal : typeSignals.second){
                    if(signal.second){
                        signal.second->pollEvents();
                    }
                }
            }
        }

    private:
        std::unordered_map<size_t, std::unordered_map<std::string, std::shared_ptr<BaseEventSignal>>> signals;
    };

}

#endif //TRIDOT_EVENTMANAGER_H

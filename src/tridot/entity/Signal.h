//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_SIGNAL_H
#define TRIDOT_SIGNAL_H

#include <functional>

namespace tridot {

    template<typename... Args>
    class SignalRef;

    template<typename... Args>
    class Signal {
    public:
        typedef std::function<void(Args...)> Callback;

        int add(const Callback &callback, const std::string &name = ""){
            listeners.emplace_back(callback, name, nextId);
            return nextId++;
        }

        int add(const std::string &name, const Callback &callback){
            listeners.emplace_back(callback, name, nextId);
            return nextId++;
        }

        void setActive(const std::string &name, bool active){
            for(int i = 0; i < listeners.size(); i++){
                auto &listener = listeners[i];
                if(listener.name == name){
                    listener.active = active;
                }
            }
        }

        void setActive(int id, bool active){
            for(int i = 0; i < listeners.size(); i++){
                auto &listener = listeners[i];
                if(listener.id == id){
                    listener.active = active;
                }
            }
        }

        void setActiveAll(bool active){
            for(int i = 0; i < listeners.size(); i++){
                auto &listener = listeners[i];
                listener.active = active;
            }
        }

        void remove(const std::string &name){
            for(int i = 0; i < listeners.size(); i++){
                auto &listener = listeners[i];
                if(listener.name == name){
                    listeners.erase(listeners.begin() + i);
                    i--;
                }
            }
        }

        void remove(int id){
            for(int i = 0; i < listeners.size(); i++){
                auto &listener = listeners[i];
                if(listener.id == id){
                    listeners.erase(listeners.begin() + i);
                    i--;
                }
            }
        }

        void invoke(Args... args){
            for(int i = 0; i < listeners.size(); i++) {
                auto &listener = listeners[i];
                if(listener.callback != nullptr){
                    if(listener.active){
                        listener.callback(args...);
                    }
                }
            }
        }

        bool order(const std::vector<std::string> &names){
            for(int i = 0; i < names.size(); i++){
                if(i != 0) {
                    for (auto &listener : listeners) {
                        if (listener.name == names[i]) {
                            listener.dependencies.push_back(names[i - 1]);
                        }
                    }
                }
            }
            return sort();
        }

        SignalRef<Args...> ref(){
            return SignalRef<Args...>(this);
        }

        void swap(Signal<Args...> &other){
            listeners.swap(other.listeners);
            std::swap(nextId, other.nextId);
        }

    private:
        class Listener{
        public:
            Callback callback;
            std::string name;
            int id;
            bool active;
            std::vector<std::string> dependencies;

            Listener(const Callback &callback = nullptr, const std::string &name = "", int id = 0, bool active = true)
                : callback(callback), name(name), id(id), active(active){}
        };
        std::vector<Listener> listeners;
        int nextId;

        bool sort(){
            bool conflict = false;
            std::vector<std::string> swaps;
            for (int i = 0; i < listeners.size(); i++) {
                int start = i;
                for (auto &dependency : listeners[i].dependencies) {
                    for (int j = i+1; j < listeners.size(); j++) {
                        if (listeners[j].name == dependency) {

                            bool loop = false;
                            for(auto &s : swaps){
                                if(s == listeners[j].name){
                                    loop = true;
                                    conflict = true;
                                    break;
                                }
                            }

                            if(!loop) {
                                listeners.insert(listeners.begin() + i, listeners[j]);
                                listeners.erase(listeners.begin() + j + 1);
                                i++;
                            }
                        }
                    }
                }
                if(i != start){
                    swaps.push_back(listeners[i].name);
                    i = start - 1;
                }else{
                    swaps.clear();
                }
            }
            return conflict;
        }
    };

    template<typename... Args>
    class SignalRef {
    public:
        typedef typename Signal<Args...>::Callback Callback;

        SignalRef(Signal<Args...> *signal) : signal(signal) {}

        int add(const Callback &callback, const std::string &name = ""){
            return signal->add(callback, name);
        }

        int add(const std::string &name, const Callback &callback){
            return signal->add(name, callback);
        }

        void setActive(const std::string &name, bool active){
            signal->setActive(name, active);
        }

        void setActive(int id, bool active){
            signal->setActive(id, active);
        }

        void setActiveAll(bool active){
            signal->setActiveAll(active);
        }

        void remove(const std::string &name){
            signal->remove(name);
        }

        void remove(int id){
            signal->remove(id);
        }

        bool order(const std::vector<std::string> &names){
            return signal->order(names);
        }

    private:
        Signal<Args...> *signal;
    };

}

#endif //TRIDOT_SIGNAL_H

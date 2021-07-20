//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "config.h"
#include "SystemManager.h"
#include "EventManager.h"
#include "Console.h"
#include "Reflection.h"

namespace tridot {

    class Time;
    class Input;
    class Physics;
    class Profiler;
    class ResourceManager;
    class Scene;

    class Window;
    class MeshRenderer;
    class PBRenderer;

    class Environment {
    public:
        //core systems
        SystemManager *systems;
        EventManager *events;
        Console *console;
        Reflection *reflection;

        //engine systems
        Time *time;
        Input *input;
        Physics *physics;
        Profiler *profiler;
        ResourceManager *resources;
        Scene *scene;

        //rendering systems
        Window *window;
        MeshRenderer *renderer;
        PBRenderer *pbRenderer;

        static Environment *init();
        static void shutdown();

    private:
        Environment();
    };
}

extern TRI_API tridot::Environment *env;

namespace tridot::impl{

    class EventCallbackRegisterer{
    public:
        int callbackId;
        EventSignal<> *signal;
        EventCallbackRegisterer(EventSignal<> &signal, const std::string &name, const std::function<void()> &callback){
            callbackId = signal.addCallback(name, callback);
            this->signal = &signal;
        }
        ~EventCallbackRegisterer(){
            signal->removeCallback(callbackId);
        }
    };

    template<typename T>
    class TypeRegisterer{
    public:
        TypeRegisterer(const std::string &name){
            Environment::init();
            env->reflection->addType<T>(name);
        }
        ~TypeRegisterer(){
            env->reflection->removeType<T>();
        }
    };

}


#define TRI_CONCAT_IMPL(a, b) a##b
#define TRI_CONCAT(a, b) TRI_CONCAT_IMPL(a, b)
#define TRI_UNIQUE_IDENTIFIER_IMPL_2(base, counter, line) base##_##line##_##counter
#define TRI_UNIQUE_IDENTIFIER_IMPL(base, counter, line) TRI_UNIQUE_IDENTIFIER_IMPL_2(base, counter, line)
#define TRI_UNIQUE_IDENTIFIER(base) TRI_UNIQUE_IDENTIFIER_IMPL(base, __COUNTER__, __LINE__)

#define TRI_REGISTER_CALLBACK_IMPL(name) void TRI_CONCAT(name, _func)(); bool TRI_CONCAT(name, _var) = [](){ TRI_CONCAT(name, _func)(); return true;}(); void TRI_CONCAT(name, _func)()
#define TRI_REGISTER_CALLBACK() TRI_REGISTER_CALLBACK_IMPL(TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_INIT_CALLBACK_IMPL(callbackName, name) void TRI_CONCAT(name, _func)(); tridot::impl::EventCallbackRegisterer TRI_CONCAT(name, _var)(tridot::Environment::init()->events->init, callbackName, &TRI_CONCAT(name, _func)); void TRI_CONCAT(name, _func)()
#define TRI_INIT_CALLBACK(callbackName) TRI_INIT_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_UPDATE_CALLBACK_IMPL(callbackName, name) void TRI_CONCAT(name, _func)(); tridot::impl::EventCallbackRegisterer TRI_CONCAT(name, _var)(tridot::Environment::init()->events->update, callbackName, &TRI_CONCAT(name, _func)); void TRI_CONCAT(name, _func)()
#define TRI_UPDATE_CALLBACK(callbackName) TRI_UPDATE_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_SHUTDOWN_CALLBACK_IMPL(callbackName, name) void TRI_CONCAT(name, _func)(); tridot::impl::EventCallbackRegisterer TRI_CONCAT(name, _var)(tridot::Environment::init()->events->shutdown, callbackName, &TRI_CONCAT(name, _func)); void TRI_CONCAT(name, _func)()
#define TRI_SHUTDOWN_CALLBACK(callbackName) TRI_SHUTDOWN_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))


#define TRI_REGISTER_TYPE_NAME(type, name) tridot::impl::TypeRegisterer<type> TRI_UNIQUE_IDENTIFIER(_tri_register_component)(#name);
#define TRI_REGISTER_TYPE(type) TRI_REGISTER_TYPE_NAME(type, type)
#define TRI_REGISTER_MEMBER(type, memberName) TRI_REGISTER_CALLBACK(){Environment::init(); env->reflection->addMember<type, decltype(type::memberName)>(#memberName, offsetof(type, memberName));}
#define TRI_REGISTER_CONSTANT(type, name) TRI_REGISTER_CALLBACK(){Environment::init(); env->reflection->addConstant<type>(#name, (int)type::name);}

#define TRI_REGISTER_MEMBER2(type, member, member2) TRI_REGISTER_MEMBER(type, member) TRI_REGISTER_MEMBER(type, member2)
#define TRI_REGISTER_MEMBER3(type, member, member2, member3) TRI_REGISTER_MEMBER(type, member) TRI_REGISTER_MEMBER(type, member2) TRI_REGISTER_MEMBER(type, member3)
#define TRI_REGISTER_MEMBER4(type, member, member2, member3, member4) TRI_REGISTER_MEMBER(type, member) TRI_REGISTER_MEMBER(type, member2) TRI_REGISTER_MEMBER(type, member3) TRI_REGISTER_MEMBER(type, member4)
#define TRI_REGISTER_MEMBER5(type, member, member2, member3, member4, member5) TRI_REGISTER_MEMBER(type, member) TRI_REGISTER_MEMBER(type, member2) TRI_REGISTER_MEMBER(type, member3) TRI_REGISTER_MEMBER(type, member4) TRI_REGISTER_MEMBER(type, member5)
#define TRI_REGISTER_MEMBER6(type, member, member2, member3, member4, member5, member6) TRI_REGISTER_MEMBER(type, member) TRI_REGISTER_MEMBER(type, member2) TRI_REGISTER_MEMBER(type, member3) TRI_REGISTER_MEMBER(type, member4) TRI_REGISTER_MEMBER(type, member5) TRI_REGISTER_MEMBER(type, member6)
#define TRI_REGISTER_MEMBER7(type, member, member2, member3, member4, member5, member6, member7) TRI_REGISTER_MEMBER(type, member) TRI_REGISTER_MEMBER(type, member2) \
    TRI_REGISTER_MEMBER(type, member3) TRI_REGISTER_MEMBER(type, member4) TRI_REGISTER_MEMBER(type, member5) TRI_REGISTER_MEMBER(type, member6) TRI_REGISTER_MEMBER(type, member7)
#define TRI_REGISTER_MEMBER8(type, member, member2, member3, member4, member5, member6, member7, member8) TRI_REGISTER_MEMBER(type, member) TRI_REGISTER_MEMBER(type, member2) \
    TRI_REGISTER_MEMBER(type, member3) TRI_REGISTER_MEMBER(type, member4) TRI_REGISTER_MEMBER(type, member5) TRI_REGISTER_MEMBER(type, member6) TRI_REGISTER_MEMBER(type, member7)  TRI_REGISTER_MEMBER(type, member8)
#define TRI_REGISTER_MEMBER9(type, member, member2, member3, member4, member5, member6, member7, member8, member9) TRI_REGISTER_MEMBER(type, member) TRI_REGISTER_MEMBER(type, member2) \
    TRI_REGISTER_MEMBER(type, member3) TRI_REGISTER_MEMBER(type, member4) TRI_REGISTER_MEMBER(type, member5) TRI_REGISTER_MEMBER(type, member6) TRI_REGISTER_MEMBER(type, member7)  TRI_REGISTER_MEMBER(type, member8) TRI_REGISTER_MEMBER(type, member9)
#define TRI_REGISTER_MEMBER10(type, member, member2, member3, member4, member5, member6, member7, member8, member9, member10) TRI_REGISTER_MEMBER(type, member) TRI_REGISTER_MEMBER(type, member2) \
    TRI_REGISTER_MEMBER(type, member3) TRI_REGISTER_MEMBER(type, member4) TRI_REGISTER_MEMBER(type, member5) TRI_REGISTER_MEMBER(type, member6) TRI_REGISTER_MEMBER(type, member7)  TRI_REGISTER_MEMBER(type, member8) TRI_REGISTER_MEMBER(type, member9) TRI_REGISTER_MEMBER(type, member10)


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

#define TRI_CONCAT_IMPL(a, b) a##b
#define TRI_CONCAT(a, b) TRI_CONCAT_IMPL(a, b)
#define TRI_UNIQUE_IDENTIFIER_IMPL_2(base, counter, line) base##_##line##_##counter
#define TRI_UNIQUE_IDENTIFIER_IMPL(base, counter, line) TRI_UNIQUE_IDENTIFIER_IMPL_2(base, counter, line)
#define TRI_UNIQUE_IDENTIFIER(base) TRI_UNIQUE_IDENTIFIER_IMPL(base, __COUNTER__, __LINE__)
#define TRI_REGISTER_CALLBACK_IMPL(name) void TRI_CONCAT(name, _func)(); bool TRI_CONCAT(name, _var) = [](){ TRI_CONCAT(name, _func)(); return true;}(); void TRI_CONCAT(name, _func)()
#define TRI_REGISTER_CALLBACK() TRI_REGISTER_CALLBACK_IMPL(TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_INIT_CALLBACK_IMPL(callbackName, name) void TRI_CONCAT(name, _func)(); bool TRI_CONCAT(name, _var) = [](){ tridot::Environment::init(); env->events->init.addCallback(callbackName, [](){TRI_CONCAT(name, _func)();}); return true;}(); void TRI_CONCAT(name, _func)()
#define TRI_INIT_CALLBACK(callbackName) TRI_INIT_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_UPDATE_CALLBACK_IMPL(callbackName, name) void TRI_CONCAT(name, _func)(); bool TRI_CONCAT(name, _var) = [](){ tridot::Environment::init(); env->events->update.addCallback(callbackName, [](){TRI_CONCAT(name, _func)();}); return true;}(); void TRI_CONCAT(name, _func)()
#define TRI_UPDATE_CALLBACK(callbackName) TRI_UPDATE_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_SHUTDOWN_CALLBACK_IMPL(callbackName, name) void TRI_CONCAT(name, _func)(); bool TRI_CONCAT(name, _var) = [](){ tridot::Environment::init(); env->events->shutdown.addCallback(callbackName, [](){TRI_CONCAT(name, _func)();}); return true;}(); void TRI_CONCAT(name, _func)()
#define TRI_SHUTDOWN_CALLBACK(callbackName) TRI_SHUTDOWN_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_ENVIRONMENT_H
#define TRIDOT_ENVIRONMENT_H

#include "config.h"
#include "SystemManager.h"
#include "EventManager.h"

namespace tridot {

    class Environment {
    public:
        SystemManager *systems;
        EventManager *events;

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
#define TRI_INIT_CALLBACK_IMPL(callbackName, name) void TRI_CONCAT(name, _func)(); bool TRI_CONCAT(name, _var) = [](){ env->events->init.addCallback(callbackName, [](){TRI_CONCAT(name, _func)();}); return true;}(); void TRI_CONCAT(name, _func)()
#define TRI_INIT_CALLBACK(callbackName) TRI_INIT_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_UPDATE_CALLBACK_IMPL(callbackName, name) void TRI_CONCAT(name, _func)(); bool TRI_CONCAT(name, _var) = [](){ env->events->update.addCallback(callbackName, [](){TRI_CONCAT(name, _func)();}); return true;}(); void TRI_CONCAT(name, _func)()
#define TRI_UPDATE_CALLBACK(callbackName) TRI_UPDATE_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_SHUTDOWN_CALLBACK_IMPL(callbackName, name) void TRI_CONCAT(name, _func)(); bool TRI_CONCAT(name, _var) = [](){ env->events->shutdown.addCallback(callbackName, [](){TRI_CONCAT(name, _func)();}); return true;}(); void TRI_CONCAT(name, _func)()
#define TRI_SHUTDOWN_CALLBACK(callbackName) TRI_SHUTDOWN_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))

#endif //TRIDOT_ENVIRONMENT_H

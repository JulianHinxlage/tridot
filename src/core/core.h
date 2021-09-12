//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "pch.h"
#include "Environment.h"
#include "SystemManager.h"
#include "Console.h"
#include "ModuleManager.h"
#include "Reflection.h"
#include "Profiler.h"
#include "SignalManager.h"
#include "ThreadPool.h"

namespace tri::impl {

    class SignalCallbackRegisterer {
    public:
        int callbackId;
        Signal<>* signal;
        SignalCallbackRegisterer(Signal<>& signal, const std::string& name, const std::function<void()>& callback) {
            callbackId = signal.addCallback(name, callback);
            this->signal = &signal;
        }
        ~SignalCallbackRegisterer() {
            signal->removeCallback(callbackId);
        }
    };

    template<typename T>
    class SystemRegisterer {
    public:
        T** instance;

        SystemRegisterer(const std::string& name, T **instance = nullptr) {
            Environment::startup();
            this->instance = instance;
            if (instance) {
                *instance = env->systems->addSystem<T>(name);
            }
            else {
                env->systems->addSystem<T>(name);
            }
        }
        ~SystemRegisterer() {
            if (env && env->systems) {
                env->systems->removeSystem<T>();
                if (instance) {
                    *instance = nullptr;
                }
            }
        }
    };

    template<typename T>
    class TypeRegisterer {
    public:
        TypeRegisterer(const std::string& name) {
            Environment::startup();
            env->reflection->registerType<T>(name);
        }
        ~TypeRegisterer() {
            if (env && env->reflection) {
                env->reflection->unregisterType<T>();
            }
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
#define TRI_STARTUP_CALLBACK_IMPL(callbackName, name) void TRI_CONCAT(name, _func)(); tri::impl::SignalCallbackRegisterer TRI_CONCAT(name, _var)(tri::Environment::startup()->signals->startup, callbackName, &TRI_CONCAT(name, _func)); void TRI_CONCAT(name, _func)()
#define TRI_STARTUP_CALLBACK(callbackName) TRI_STARTUP_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_UPDATE_CALLBACK_IMPL(callbackName, name) void TRI_CONCAT(name, _func)(); tri::impl::SignalCallbackRegisterer TRI_CONCAT(name, _var)(tri::Environment::startup()->signals->update, callbackName, &TRI_CONCAT(name, _func)); void TRI_CONCAT(name, _func)()
#define TRI_UPDATE_CALLBACK(callbackName) TRI_UPDATE_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_SHUTDOWN_CALLBACK_IMPL(callbackName, name) void TRI_CONCAT(name, _func)(); tri::impl::SignalCallbackRegisterer TRI_CONCAT(name, _var)(tri::Environment::startup()->signals->shutdown, callbackName, &TRI_CONCAT(name, _func)); void TRI_CONCAT(name, _func)()
#define TRI_SHUTDOWN_CALLBACK(callbackName) TRI_SHUTDOWN_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))

#define TRI_REGISTER_SYSTEM(type) tri::impl::SystemRegisterer<type> TRI_UNIQUE_IDENTIFIER(_tri_register_component)(#type);
#define TRI_REGISTER_SYSTEM_INSTANCE(type, instance) TRI_REGISTER_CALLBACK(){tri::Environment::startup();} tri::impl::SystemRegisterer<type> TRI_UNIQUE_IDENTIFIER(_tri_register_component)(#type, &instance);

#define TRI_REGISTER_TYPE_NAME(type, name) tri::impl::TypeRegisterer<type> TRI_UNIQUE_IDENTIFIER(_tri_register_component)(#name);
#define TRI_REGISTER_TYPE(type) TRI_REGISTER_TYPE_NAME(type, type)
#define TRI_REGISTER_MEMBER(type, memberName) TRI_REGISTER_CALLBACK(){tri::Environment::startup(); env->reflection->registerMember<type, decltype(type::memberName)>(#memberName, offsetof(type, memberName));}
#define TRI_REGISTER_CONSTANT(type, name) TRI_REGISTER_CALLBACK(){tri::Environment::startup(); env->reflection->registerConstant<type>(#name, (int)type::name);}


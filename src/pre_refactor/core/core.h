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
#include "JobSystem.h"
#include <glm/glm.hpp>

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
            if (env && env->signals) {
                signal->removeCallback(callbackId);
            }
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
        TypeRegisterer(const std::string& name, const std::string& group, Reflection::TypeFlags flags = Reflection::NONE) {
            Environment::startup();
            env->reflection->registerType<T>(name, group, flags);
        }
        ~TypeRegisterer() {
            if (env && env->reflection) {
                env->reflection->unregisterType<T>();
            }
        }
    };

}

//helper macros
#define TRI_CONCAT_IMPL(a, b) a##b
#define TRI_CONCAT(a, b) TRI_CONCAT_IMPL(a, b)
#define TRI_UNIQUE_IDENTIFIER_IMPL_2(base, counter, line) base##_##line##_##counter
#define TRI_UNIQUE_IDENTIFIER_IMPL(base, counter, line) TRI_UNIQUE_IDENTIFIER_IMPL_2(base, counter, line)
#define TRI_UNIQUE_IDENTIFIER(base) TRI_UNIQUE_IDENTIFIER_IMPL(base, __COUNTER__, __LINE__)

//callback macros
#define TRI_REGISTER_CALLBACK_IMPL(name) static void TRI_CONCAT(name, _func)(); namespace{ static bool TRI_CONCAT(name, _var) = [](){ TRI_CONCAT(name, _func)(); return true;}();} void TRI_CONCAT(name, _func)()
#define TRI_REGISTER_CALLBACK() TRI_REGISTER_CALLBACK_IMPL(TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_STARTUP_CALLBACK_IMPL(callbackName, name) static void TRI_CONCAT(name, _func)(); namespace{ static tri::impl::SignalCallbackRegisterer TRI_CONCAT(name, _var)(tri::Environment::startup()->signals->startup, callbackName, &TRI_CONCAT(name, _func));} void TRI_CONCAT(name, _func)()
#define TRI_STARTUP_CALLBACK(callbackName) TRI_STARTUP_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_UPDATE_CALLBACK_IMPL(callbackName, name) static void TRI_CONCAT(name, _func)(); namespace{ static tri::impl::SignalCallbackRegisterer TRI_CONCAT(name, _var)(tri::Environment::startup()->signals->update, callbackName, &TRI_CONCAT(name, _func));} void TRI_CONCAT(name, _func)()
#define TRI_UPDATE_CALLBACK(callbackName) TRI_UPDATE_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))
#define TRI_SHUTDOWN_CALLBACK_IMPL(callbackName, name) static void TRI_CONCAT(name, _func)(); namespace{ static tri::impl::SignalCallbackRegisterer TRI_CONCAT(name, _var)(tri::Environment::startup()->signals->shutdown, callbackName, &TRI_CONCAT(name, _func));} void TRI_CONCAT(name, _func)()
#define TRI_SHUTDOWN_CALLBACK(callbackName) TRI_SHUTDOWN_CALLBACK_IMPL(callbackName, TRI_UNIQUE_IDENTIFIER(_tri_register_callback))


//register macros
#define TRI_REGISTER_SYSTEM(type) namespace{ static tri::impl::SystemRegisterer<type> TRI_UNIQUE_IDENTIFIER(_tri_register_system)(#type);}
#define TRI_REGISTER_SYSTEM_INSTANCE(type, instance) namespace{TRI_REGISTER_CALLBACK(){tri::Environment::startup();} static tri::impl::SystemRegisterer<type> TRI_UNIQUE_IDENTIFIER(_tri_register_system)(#type, &instance);}

#define TRI_REGISTER_TYPE_FLAGS(type, name, group, flags) namespace{ static tri::impl::TypeRegisterer<type> TRI_UNIQUE_IDENTIFIER(_tri_register_type)(#name, group, flags);}
#define TRI_REGISTER_TYPE_NAME(type, name, group) TRI_REGISTER_TYPE_FLAGS(type, name, group, tri::Reflection::NONE)
#define TRI_REGISTER_TYPE_GROUP(type, group) TRI_REGISTER_TYPE_NAME(type, type, group)
#define TRI_REGISTER_TYPE(type) TRI_REGISTER_TYPE_GROUP(type, "")

#define TRI_REGISTER_COMPONENT_NAME(type, name, group) TRI_REGISTER_TYPE_FLAGS(type, name, group, tri::Reflection::COMPONENT)
#define TRI_REGISTER_COMPONENT_GROUP(type, group) TRI_REGISTER_COMPONENT_NAME(type, type, group)
#define TRI_REGISTER_COMPONENT(type) TRI_REGISTER_COMPONENT_NAME(type, type, "")
#define TRI_REGISTER_COMPONENT_UPDATE(type, func) TRI_UPDATE_CALLBACK(#type) { env->scene->view<type>().each([](type &c){ c.update(); }); }

#define TRI_REGISTER_ASSET_NAME(type, name) TRI_REGISTER_TYPE_FLAGS(type, name, "", tri::Reflection::ASSET)
#define TRI_REGISTER_ASSET(type) TRI_REGISTER_ASSET_NAME(type, type)

#define TRI_REGISTER_MEMBER_FLAGS_RANGE(type, memberName, flags, min, max) TRI_REGISTER_CALLBACK(){tri::Environment::startup(); env->reflection->registerMember<type, decltype(type::memberName)>(#memberName, offsetof(type, memberName), flags, min, max);}
#define TRI_REGISTER_MEMBER_FLAGS(type, memberName, flags) TRI_REGISTER_CALLBACK(){tri::Environment::startup(); env->reflection->registerMember<type, decltype(type::memberName)>(#memberName, offsetof(type, memberName), flags);}
#define TRI_REGISTER_MEMBER_RANGE(type, memberName, min, max) TRI_REGISTER_MEMBER_FLAGS_RANGE(type, memberName, Reflection::NONE, min, max)
#define TRI_REGISTER_MEMBER(type, memberName) TRI_REGISTER_MEMBER_FLAGS(type, memberName, Reflection::NONE)

#define TRI_REGISTER_CONSTANT(type, name) TRI_REGISTER_CALLBACK(){tri::Environment::startup(); env->reflection->registerConstant<type>(#name, (int)type::name);}


//multi register macros
#define TRI_REGISTER_MEMBER_1(type, m1) TRI_REGISTER_MEMBER(type, m1)
#define TRI_REGISTER_MEMBER_2(type, m1, m2) TRI_REGISTER_MEMBER_1(type, m1) TRI_REGISTER_MEMBER(type, m2)
#define TRI_REGISTER_MEMBER_3(type, m1, m2, m3) TRI_REGISTER_MEMBER_2(type, m1, m2) TRI_REGISTER_MEMBER(type, m3)
#define TRI_REGISTER_MEMBER_4(type, m1, m2, m3, m4) TRI_REGISTER_MEMBER_3(type, m1, m2, m3) TRI_REGISTER_MEMBER(type, m4)
#define TRI_REGISTER_MEMBER_5(type, m1, m2, m3, m4, m5) TRI_REGISTER_MEMBER_4(type, m1, m2, m3, m4) TRI_REGISTER_MEMBER(type, m5)
#define TRI_REGISTER_MEMBER_6(type, m1, m2, m3, m4, m5, m6) TRI_REGISTER_MEMBER_5(type, m1, m2, m3, m4, m5) TRI_REGISTER_MEMBER(type, m6)
#define TRI_REGISTER_MEMBER_7(type, m1, m2, m3, m4, m5, m6, m7) TRI_REGISTER_MEMBER_6(type, m1, m2, m3, m4, m5, m6) TRI_REGISTER_MEMBER(type, m7)
#define TRI_REGISTER_MEMBER_8(type, m1, m2, m3, m4, m5, m6, m7, m8) TRI_REGISTER_MEMBER_7(type, m1, m2, m3, m4, m5, m6, m7) TRI_REGISTER_MEMBER(type, m8)

#define TRI_REGISTER_CONSTANT_1(type, name) TRI_REGISTER_CONSTANT(type, name)
#define TRI_REGISTER_CONSTANT_2(type, n1, n2) TRI_REGISTER_CONSTANT_1(type, n1), TRI_REGISTER_CONSTANT(type, n2)
#define TRI_REGISTER_CONSTANT_3(type, n1, n2, n3) TRI_REGISTER_CONSTANT_2(type, n1, n2), TRI_REGISTER_CONSTANT(type, n3)
#define TRI_REGISTER_CONSTANT_4(type, n1, n2, n3, n4) TRI_REGISTER_CONSTANT_3(type, n1, n2, n3), TRI_REGISTER_CONSTANT(type, n4)
#define TRI_REGISTER_CONSTANT_5(type, n1, n2, n3, n4, n5) TRI_REGISTER_CONSTANT_4(type, n1, n2, n3, n4), TRI_REGISTER_CONSTANT(type, n5)
#define TRI_REGISTER_CONSTANT_6(type, n1, n2, n3, n4, n5, n6) TRI_REGISTER_CONSTANT_5(type, n1, n2, n3, n4, n5), TRI_REGISTER_CONSTANT(type, n6)
#define TRI_REGISTER_CONSTANT_7(type, n1, n2, n3, n4, n5, n6, n7) TRI_REGISTER_CONSTANT_6(type, n1, n2, n3, n4, n5, n6), TRI_REGISTER_CONSTANT(type, n7)
#define TRI_REGISTER_CONSTANT_8(type, n1, n2, n3, n4, n5, n6, n7, n8) TRI_REGISTER_CONSTANT_7(type, n1, n2, n3, n4, n5, n6, n7), TRI_REGISTER_CONSTANT(type, n8)

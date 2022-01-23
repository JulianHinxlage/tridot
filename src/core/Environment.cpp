//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Environment.h"
#include "SystemManager.h"
#include "Console.h"
#include "ModuleManager.h"
#include "Reflection.h"
#include "Profiler.h"
#include "SignalManager.h"
#include "ThreadPool.h"

tri::Environment* env = tri::Environment::startup();

namespace tri {

    static bool& getInitFlag() {
        static bool init = false;
        return init;
    }

    static Environment*& getInstance() {
        static Environment* instance = nullptr;
        return instance;
    }

    Environment::Environment() {
        systems = nullptr;
        console = nullptr;
        modules = nullptr;
        reflection = nullptr;
        profiler = nullptr;
        signals = nullptr;
        threads = nullptr;
        scene = nullptr;
        input = nullptr;
        time = nullptr;
        window = nullptr;
        renderer = nullptr;
        serializer = nullptr;
        assets = nullptr;
        editor = nullptr;
        hierarchies = nullptr;
        physics = nullptr;
    }

    Environment* Environment::startup() {
        if (!getInitFlag()) {
            getInitFlag() = true;
            getInstance() = new Environment();
            env = getInstance();
            env->systems = new SystemManager();
            env->reflection = new Reflection();
            env->signals = new SignalManager();
            env->console = new Console();
            env->modules = new ModuleManager();
            env->systems->setSystem("SignalManager", env->signals);
            env->systems->setSystem("Console", env->console);
            env->systems->setSystem("Reflection", env->reflection);
            env->profiler = env->systems->addSystem<Profiler>("Profiler");
            env->threads = env->systems->addSystem<ThreadPool>("ThreadPool");

            env->signals->startup.addCallback("systems", []() {
                env->systems->startup();
                env->modules->startup();
            });
            env->signals->shutdown.addCallback("systems", []() {
                //todo: fix unloading modules on shutdown causing a crash
                //env->modules->shutdown();
                env->systems->shutdown();
            });
        }
        else {
            env = getInstance();
        }
        return getInstance();
    }

    void Environment::shutdown() {
        env = getInstance();
        if (env != nullptr) {
            delete env->modules;
            delete env->signals;
            delete env->reflection;
            delete env->console;
            delete env->systems;
            delete env;
            getInstance() = nullptr;
        }
        env = nullptr;
        getInitFlag() = false;
    }

}
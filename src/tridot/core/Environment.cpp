//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Environment.h"

tridot::Environment *env = tridot::Environment::init();

namespace tridot {

    Environment::Environment() {
        //core systems
        systems = nullptr;
        events = nullptr;
        console = nullptr;
        reflection = nullptr;

        //engine system
        time = nullptr;
        input = nullptr;
        physics = nullptr;
        profiler = nullptr;
        resources = nullptr;
        scene = nullptr;
        editor = nullptr;

        //rendering system
        window = nullptr;
        renderer = nullptr;
        pbRenderer = nullptr;
    }

    static bool &getInitFlag(){
        static bool init = false;
        return init;
    }

    static Environment *&getInstance(){
        static Environment *instance = nullptr;
        return instance;
    }

    Environment *Environment::init() {
        if(!getInitFlag()){
            getInitFlag() = true;
            getInstance() = new Environment();
            env = getInstance();
            env->systems = new SystemManager();
            env->systems->setSystem<Environment>(env);
            env->systems->setSystem<SystemManager>(env->systems);
            env->events = env->systems->addSystem<EventManager>();
            env->console = env->systems->addSystem<Console>();
            env->reflection = env->systems->addSystem<Reflection>();
        }else{
            env = getInstance();
        }
        return getInstance();
    }

    void Environment::shutdown() {
        env = getInstance();
        if(env != nullptr){
            delete env->systems;
            delete env;
            getInstance() = nullptr;
        }
        env = nullptr;
        getInitFlag() = false;
    }

}

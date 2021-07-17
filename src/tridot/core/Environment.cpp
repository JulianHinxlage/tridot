//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Environment.h"

tridot::Environment *env = tridot::Environment::init();

namespace tridot {

    Environment::Environment() {

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
            env->events->init.addCallback("env", [](){

            });
        }else{
            env = getInstance();
        }
        return getInstance();
    }

    void Environment::shutdown() {
        env = getInstance();
        if(env != nullptr){
            if(env->events){
                env->events->init.removeCallback("env");
            }
            delete env->systems;
            delete env;
            getInstance() = nullptr;
        }
        env = nullptr;
        getInitFlag() = false;
    }

}

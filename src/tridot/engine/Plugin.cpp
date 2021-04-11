//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Plugin.h"
#include "Engine.h"
#include "tridot/core/Log.h"
#include <dlfcn.h>

namespace tridot {

    Plugin::Plugin() {
        handle = nullptr;
        file = "";
        init = nullptr;
        update = nullptr;
        shutdown = nullptr;
    }

    Plugin::~Plugin() {
        unload();
    }

    bool Plugin::preLoad(const std::string &file) {
        this->file = file;
        unload();
        handle = dlopen(this->file.c_str(), RTLD_NOW | RTLD_LOCAL);
        if(handle){
            Log::trace("loaded plugin ", this->file);
            typedef void (*Function)();
            init = (Function)dlsym(handle, "init");
            update = (Function)dlsym(handle, "update");
            shutdown = (Function)dlsym(handle, "shutdown");
            if(init){
                init();
            }
        }else{
            Log::warning("failed to load plugin ", dlerror());
        }
        return handle != nullptr;
    }

    bool Plugin::postLoad() {
        return true;
    }

    void Plugin::unload() {
        if(handle != nullptr){
            if(shutdown){
                shutdown();
            }
            dlclose(handle);
            init = nullptr;
            update = nullptr;
            shutdown = nullptr;
            handle = nullptr;
            Log::trace("unloaded plugin ", this->file);
        }
    }

    TRI_UPDATE("plugins"){
        for(auto &name : engine.resources.getNames<Plugin>()){
            auto plugin = engine.resources.get<Plugin>(name);
            if(plugin){
                if(plugin->update){
                    plugin->update();
                }
            }
        }
    }

}


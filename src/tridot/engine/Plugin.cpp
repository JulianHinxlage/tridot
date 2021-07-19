//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Plugin.h"
#include "tridot/core/Environment.h"
#include "tridot/engine/ResourceManager.h"
#include "tridot/core/Log.h"
#if WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

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
        return true;
    }

    bool Plugin::postLoad() {
#if WIN32
        unload();
        handle = (void*)LoadLibrary(file.c_str());
        if (handle) {
            Log::debug("loaded plugin ", this->file);
            typedef void (*Function)();
            init = (Function)GetProcAddress((HINSTANCE)handle, "init");
            update = (Function)GetProcAddress((HINSTANCE)handle, "update");
            shutdown = (Function)GetProcAddress((HINSTANCE)handle, "shutdown");
            if (!update) {
                Log::warning("no update function found in plugin ", file);
            }
            if (init) {
                init();
            }
        } else {
            Log::warning("failed to load plugin ", file, " code ", GetLastError());
        }
        return handle != nullptr;
#else
        unload();
        handle = dlopen(this->file.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (handle) {
            Log::debug("loaded plugin ", this->file);
            typedef void (*Function)();
            init = (Function)dlsym(handle, "init");
            update = (Function)dlsym(handle, "update");
            shutdown = (Function)dlsym(handle, "shutdown");
            if (init) {
                init();
            }
        }
        else {
            Log::warning("failed to load plugin ", dlerror());
        }
        return handle != nullptr;
#endif
    }

    void Plugin::unload() {
        if (handle != nullptr) {
            if (shutdown) {
                shutdown();
            }
#if WIN32
            FreeLibrary((HINSTANCE)handle);
#else      
            dlclose(handle);
#endif            
            init = nullptr;
            update = nullptr;
            shutdown = nullptr;
            handle = nullptr;
            Log::debug("unloaded plugin ", this->file);
        }
    }

    TRI_UPDATE_CALLBACK("plugins"){
        for(auto &name : env->resources->getNameList<Plugin>()){
            auto plugin = env->resources->get<Plugin>(name);
            if(plugin){
                if(plugin->update){
                    plugin->update();
                }
            }
        }
    }

    TRI_INIT_CALLBACK("plugins"){
        env->events->shutdown.addCallback("plugins", [](){
            for(auto &name : env->resources->getNameList<Plugin>()){
                auto plugin = env->resources->get<Plugin>(name);
                if(plugin){
                    plugin->init = nullptr;
                    plugin->update = nullptr;
                    plugin->shutdown = nullptr;
                    plugin->handle = nullptr;
                }
            }
        });
    }

}


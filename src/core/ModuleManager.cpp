//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "config.h"
#include "ModuleManager.h"
#include "Environment.h"
#include "Console.h"
#include "SignalManager.h"

#if !TRI_WINDOWS
#include <dlfcn.h>
#endif

namespace tri {

    ModuleManager::ModuleManager(){
        startupFlag = false;
    }

    void ModuleManager::startup(){
        startupFlag = true;
        for (auto& record : modules) {
            if (record.second.module) {
                if (record.second.startupFlag == false) {
                    record.second.startupFlag = true;
                    record.second.module->startup();
                }
            }
        }
    }

    void ModuleManager::update(){

    }

    void ModuleManager::shutdown(){
        while (!modules.empty()) {
            for (auto &record : modules) {
                if (record.second.module) {
                    unloadModule(record.second.module);
                }
                else {
                    modules.erase(record.first);
                }
                break;
            }
        }
    }

    Module* ModuleManager::loadModule(const std::string& file){
        if (modules.find(file) != modules.end()) {
            env->console->warning("module ", file, " already loaded");
            return modules[file].module;
        }
#if TRI_WINDOWS
        void* handle = (void*)LoadLibrary(file.c_str());
#else
        void* handle = dlopen(file.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
        if (handle) {
            env->console->debug("loaded module ", file);
            typedef Module* (*Function)();

#if TRI_WINDOWS
            Function create = (Function)GetProcAddress((HINSTANCE)handle, "triCreateModuleInstance");
#else
            Function create = (Function)dlsym(handle, "triCreateModuleInstance");
#endif
            Module* module = nullptr;
            if (create) {
                module = create();
            }
            else {
#if TRI_WINDOWS
                env->console->warning("no module class defined in module ", file, " (code ", GetLastError(), ")");
#else
                env->console->warning("no module class defined in module ", file, " (", dlerror(), ")");
#endif
            }
            auto pos1 = file.find_last_of('.');
            if (pos1 == std::string::npos) {
                pos1 = file.size();
            }
            auto pos2 = file.find_last_of('/');
            if (pos2 == std::string::npos) {
                pos2 = file.find_last_of('\\');
                if (pos2 == std::string::npos) {
                    pos2 = 0;
                }
                else {
                    pos2++;
                }
            }
            else {
                pos2++;
            }
            std::string name = file.substr(pos2, pos1 - pos2);
            int callbackId = -1;
            if (module) {
                callbackId = env->signals->update.addCallback(name, [module]() { module->update(); });
            }
            if (module && startupFlag == true) {
                module->startup();
                modules[file] = { module, handle, true };
            }
            else {
                modules[file] = { module, handle, false };
            }
            modules[file].updateCallbackId = callbackId;
            return module;
        }
        else {
#if TRI_WINDOWS
            env->console->warning("failed to load module ", file, " (code ", GetLastError(), ")");
#else
            env->console->warning("failed to load module ", file, " (", dlerror(), ")");
#endif
            return nullptr;
        }
    }

    Module* ModuleManager::getModule(const std::string& file){
        if (modules.contains(file)) {
            return modules[file].module;
        }
        else {
            return nullptr;
        }
    }

    void ModuleManager::unloadModule(Module* module){
        if (module) {
            for (auto it = modules.begin(); it != modules.end();) {
                if (it->second.module == module) {
                    it->second.module->shutdown();
                    env->signals->update.removeCallback(it->second.updateCallbackId);
                    delete it->second.module;
#ifdef TRI_WINDOWS
                    FreeLibrary((HINSTANCE)it->second.handle);
#else      
                    dlclose(it->second.handle);
#endif
                    env->console->debug("unloaded module ", it->first);
                    modules.erase(it++);
                }
                else {
                    ++it;
                }
            }
        }
    }

}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "config.h"
#include "ModuleManager.h"
#include "Environment.h"
#include "Console.h"
#include "SignalManager.h"
#include "engine/AssetManager.h"
#include "engine/Time.h"

#if !TRI_WINDOWS
#include <dlfcn.h>
#endif

namespace tri {

    uint64_t getTimeStamp(const std::string& file);

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
        if (env->assets->hotReloadEnabled) {
            if (env->time->frameTicks(1.0)) {
                for (auto& record : modules) {
                    uint64_t currentTimeStamp = getTimeStamp(record.second.file);
                    if (currentTimeStamp != record.second.timeStamp) {
                        std::string file = record.second.file;
                        record.second.timeStamp = currentTimeStamp;
                        unloadModule(record.second.module);
                        loadModule(file);
                        break;
                    }
                }
            }
        }
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
        if (modules.find(file) != modules.end() && modules.find(file)->second.module != nullptr) {
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
                env->signals->moduleLoad.invoke(module);
            }
            if (module && startupFlag == true) {
                module->startup();
                modules[file] = { name, file, module, handle, true };
            }
            else {
                modules[file] = { name, file, module, handle, false };
            }
            modules[file].updateCallbackId = callbackId;
            modules[file].timeStamp = getTimeStamp(file);
            return module;
        }
        else {

            modules[file] = { "", file, nullptr, handle, true };
            modules[file].timeStamp = getTimeStamp(file);

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
                    env->signals->moduleUnload.invoke(it->second.module);
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

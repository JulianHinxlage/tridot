//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "pch.h"
#include "System.h"
#include "Module.h"

namespace tri {

    class ModuleManager : public System {
    public:
        ModuleManager();

        virtual void startup() override;
        virtual void update() override;
        virtual void shutdown() override;

        Module* loadModule(const std::string& file);
        Module* getModule(const std::string& file);
        void unloadModule(Module* module);

    private:
        class ModuleRecord {
        public:
            std::string name;
            std::string file;
            Module* module;
            void* handle;
            bool startupFlag;
            int updateCallbackId;
            uint64_t timeStamp;
        };
        std::unordered_map<std::string, ModuleRecord> modules;
        bool startupFlag;

    };

}

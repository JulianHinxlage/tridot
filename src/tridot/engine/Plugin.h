//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <string>
#include <functional>

namespace tridot {

    class Plugin {
    public:
        void *handle;
        std::string file;
        std::function<void()> init;
        std::function<void()> update;
        std::function<void()> shutdown;

        Plugin();
        ~Plugin();

        bool preLoad(const std::string &file);
        bool postLoad();
        void unload();
    };

}


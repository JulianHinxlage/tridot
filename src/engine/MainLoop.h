//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"

namespace tri {

    class MainLoop {
    public:
        void startup(const std::string &configFile, const std::string& fallbackConfigFile = "");
        void run();
        void shutdown();
    };

}


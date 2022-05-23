//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"

namespace tri {

    class MainLoop {
    public:
        void startup(const std::vector<std::string>& configFileList, const std::string &defaultTitle = "Tridot");
        void run();
        void shutdown();
    };

}


//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"

namespace tri {

    class MainLoop {
    public:
        void startup(const std::vector<std::string>& configFileList);
        void run();
        void shutdown();
    };

}


//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <string>

namespace tridot {

    class Audio {
    public:
        Audio();
        ~Audio();
        bool preLoad(const std::string &file);
        bool postLoad();

        void clear();
        uint32_t getId();
    private:
        uint32_t id;
    };

}


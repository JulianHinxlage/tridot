//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <string>

namespace tridot {

    class Tag {
    public:
        std::string tag;
        Tag(const std::string &tag = "") : tag(tag){}
    };

    class uuid{
    public:
        uint64_t v1;
        uint64_t v2;

        uuid();
        std::string str();
        void set(const std::string &str);
        void make();
    };

}


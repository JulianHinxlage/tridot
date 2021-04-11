//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_TAG_H
#define TRIDOT_TAG_H

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

#endif //TRIDOT_TAG_H

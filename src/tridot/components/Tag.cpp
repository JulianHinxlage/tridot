//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Tag.h"
#include <sstream>
#include <iomanip>

bool initRand = [](){
    std::srand(std::time(nullptr));
    return true;
}();

namespace tridot{

    uuid::uuid() {
        make();
    }

    std::string uuid::str() {
        std::stringstream s;
        s << std::hex << std::setw(sizeof(v1) * 2) << std::setfill('0') << v1;
        s << std::hex << std::setw(sizeof(v2) * 2) << std::setfill('0') << v2;
        return s.str();
    }

    void uuid::set(const std::string &str) {
        {
            std::stringstream s;
            s << str.substr(0, str.size() / 2);
            s << std::hex;
            s >> v1;
        }
        {
            std::stringstream s;
            s << str.substr(str.size() / 2);
            s << std::hex;
            s >> v2;
        }
    }

    void uuid::make() {
        *((uint16_t*)&v1 + 0) = std::rand();
        *((uint16_t*)&v1 + 1) = std::rand();
        *((uint16_t*)&v1 + 2) = std::rand();
        *((uint16_t*)&v1 + 3) = std::rand();

        *((uint16_t*)&v2 + 0) = std::rand();
        *((uint16_t*)&v2 + 1) = std::rand();
        *((uint16_t*)&v2 + 2) = std::rand();
        *((uint16_t*)&v2 + 3) = std::rand();
    }

}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_SERIALIZER_H
#define TRIDOT_SERIALIZER_H

#include "tridot/ecs/Registry.h"
#include "ResourceManager.h"

namespace tridot {

    class Serializer {
    public:
        bool save(const std::string &file, ecs::Registry &reg, ResourceManager &resources, bool binary = false);
        bool load(const std::string &file, ecs::Registry &reg, ResourceManager &resources);
    };

}

#endif //TRIDOT_SERIALIZER_H

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_SERIALIZER_H
#define TRIDOT_SERIALIZER_H

#include "tridot/entity/Registry.h"
#include "tridot/core/Environment.h"
#include "ResourceManager.h"
#include <yaml-cpp/yaml.h>

namespace tridot {

    class Serializer {
    public:
        bool save(const std::string &file, Registry &reg, ResourceManager &resources, bool binary = false);
        bool load(const std::string &file, Registry &reg, ResourceManager &resources);

        void serializeEntity(EntityId id, YAML::Emitter& out, Registry& reg, ResourceManager& resources);
        EntityId deserializeEntity(YAML::Node& in, Registry& reg, ResourceManager& resources);

        void serializeType(Reflection::TypeDescriptor *type, const std::string &name, YAML::Emitter &out, void *ptr, ResourceManager &resources);
        void deserializeType(Reflection::TypeDescriptor *type, YAML::Node &in, void *ptr, ResourceManager &resources);
    };

}

#endif //TRIDOT_SERIALIZER_H

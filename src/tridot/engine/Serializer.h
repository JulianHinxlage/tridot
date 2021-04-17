//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_SERIALIZER_H
#define TRIDOT_SERIALIZER_H

#include "tridot/ecs/Registry.h"
#include "ResourceManager.h"
#include <yaml-cpp/yaml.h>

namespace tridot {

    class Serializer {
    public:
        bool save(const std::string &file, ecs::Registry &reg, ResourceManager &resources, bool binary = false);
        bool load(const std::string &file, ecs::Registry &reg, ResourceManager &resources);

        void serializeEntity(ecs::EntityId id, YAML::Emitter& out, ecs::Registry& reg, ResourceManager& resources);
        ecs::EntityId deserializeEntity(YAML::Node& in, ecs::Registry& reg, ResourceManager& resources);

        void serializeType(ecs::Reflection::Type *type, const std::string &name, YAML::Emitter &out, void *ptr, ResourceManager &resources);
        void deserializeType(ecs::Reflection::Type *type, YAML::Node &in, void *ptr, ResourceManager &resources);
    };

}

#endif //TRIDOT_SERIALIZER_H

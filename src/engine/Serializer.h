//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "entity/Scene.h"
#include "entity/Prefab.h"
#include <yaml-cpp/yaml.h>

namespace tri {

    class Serializer : public System {
    public:
        void serializeType(YAML::Emitter &out, int typeId, void *data);
        void deserializeType(YAML::Node &in, int typeId, void *data);

        void serializeEntity(YAML::Emitter &out, Scene &scene, EntityId id);
        void deserializeEntity(YAML::Node &in, Scene &scene);

        void serializePrefab(YAML::Emitter &out, Prefab &prefab);
        void deserializePrefab(YAML::Node &in, Prefab &prefab);

        void serializeScene(YAML::Emitter &out, Scene &scene);
        void deserializeScene(YAML::Node &in, Scene &scene);

        bool serializeType(const std::string &filename, int typeId, void *data);
        bool deserializeType(const std::string &filename, int typeId, void *data);
        bool serializeScene(const std::string &filename, Scene &scene);
        bool deserializeScene(const std::string &filename, Scene &scene);

        template<typename T>
        void setSerializationFunction(const std::function<void(YAML::Emitter &, T&)> &func){
            setSerializationFunction(env->reflection->getTypeId<T>(), [func](YAML::Emitter &out, void *data){
                func(out, *(T*)data);
            });
        }

        template<typename T>
        void setDeserializationFunction(const std::function<void(YAML::Node &, T&)> &func){
            setDeserializationFunction(env->reflection->getTypeId<T>(), [func](YAML::Node &in, void *data){
                func(in, *(T*)data);
            });
        }

        void startup() override;

    private:
        void setSerializationFunction(int typeId, const std::function<void(YAML::Emitter &, void*)> &func);
        void setDeserializationFunction(int typeId, const std::function<void(YAML::Node &, void*)> &func);

        std::vector<std::function<void(YAML::Emitter &, void*)>> serializationFunctions;
        std::vector<std::function<void(YAML::Node &, void*)>> deserializationFunctions;
    };

}


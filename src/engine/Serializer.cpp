//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Serializer.h"
#include "render/Color.h"
#include "render/Mesh.h"
#include "AssetManager.h"
#include "render/Material.h"
#include <glm/glm.hpp>

namespace tri {

    TRI_REGISTER_SYSTEM_INSTANCE(Serializer, env->serializer);

    void Serializer::serializeType(YAML::Emitter &out, int typeId, void *data) {
        if(serializationFunctions.size() > typeId){
            if(serializationFunctions[typeId]){
                serializationFunctions[typeId](out, data);
                return;
            }
        }

        auto *desc = env->reflection->getType(typeId);

        if(desc->constants.size() > 0){
            int value = *(int*)data;
            for(auto &c : desc->constants){
                if(c.value == value){
                    out << YAML::Value << c.name;
                    return;
                }
            }
        }

        if(desc->member.size() > 0){
            out << YAML::Value << YAML::BeginMap;
            for(auto &m : desc->member){
                out << YAML::Key << m.name;
                serializeType(out, m.type->typeId, (uint8_t*)data + m.offset);
            }
            out << YAML::EndMap;
        }else{
            out << YAML::Value << YAML::Null;
        }
    }

    void Serializer::deserializeType(YAML::Node &in, int typeId, void *data) {
        if(deserializationFunctions.size() > typeId){
            if(deserializationFunctions[typeId]){
                deserializationFunctions[typeId](in, data);
                return;
            }
        }
        auto *desc = env->reflection->getType(typeId);

        if(desc->constants.size() > 0){
            int &value = *(int*)data;
            std::string name = in.Scalar();
            for(auto &c : desc->constants){
                if(c.name == name){
                    value = c.value;
                    return;
                }
            }
        }

        if(desc->member.size() > 0){
            for(auto &m : desc->member){
                YAML::Node type = in[m.name];
                if(type){
                    deserializeType(type, m.type->typeId, (uint8_t*)data + m.offset);
                }
            }
        }
    }

    void Serializer::serializeScene(YAML::Emitter &out, Scene &scene) {
        out << YAML::BeginSeq;
        scene.view<>().each([&](EntityId id){
            out << YAML::BeginMap;
            out << YAML::Key << "id" << YAML::Value << id;
            for(auto &desc : env->reflection->getDescriptors()){
                if(scene.hasComponent(desc->typeId, id)){
                    out << YAML::Key << desc->name;
                    serializeType(out, desc->typeId, scene.getComponent(desc->typeId, id));
                }
            }
            out << YAML::EndMap;
        });
        out << YAML::EndSeq;
    }

    void Serializer::deserializeScene(YAML::Node &in, Scene &scene) {
        for(auto entity : in){
            EntityId id = entity["id"].as<EntityId>(-1);
            id = scene.addEntityHinted(id);
            for(auto &desc : env->reflection->getDescriptors()){
                YAML::Node component = entity[desc->name];
                if(component){
                    void *data = scene.addComponent(desc->typeId, id);
                    deserializeType(component, desc->typeId, data);
                }
            }
        }
    }

    bool Serializer::serializeType(const std::string &filename, int typeId, void *data) {
        std::ofstream stream(filename);
        if(stream.is_open()){
            YAML::Emitter out(stream);
            serializeType(out, typeId, data);
            return true;
        }
        return false;
    }

    bool Serializer::deserializeType(const std::string &filename, int typeId, void *data) {
        std::ifstream stream(filename);
        if(stream.is_open()){
            YAML::Node in = YAML::Load(stream);
            deserializeType(in, typeId, data);
            return true;
        }
        return false;
    }

    bool Serializer::serializeScene(const std::string &filename, Scene &scene) {
        std::ofstream stream(filename);
        if(stream.is_open()){
            YAML::Emitter out(stream);
            serializeScene(out, scene);
            return true;
        }
        return false;
    }

    bool Serializer::deserializeScene(const std::string &filename, Scene &scene) {
        std::ifstream stream(filename);
        if(stream.is_open()){
            YAML::Node in = YAML::Load(stream);
            deserializeScene(in, scene);
            return true;
        }
        return false;
    }

    void Serializer::setSerializationFunction(int typeId, const std::function<void(YAML::Emitter &, void *)> &func) {
        if(serializationFunctions.size() <= typeId){
            serializationFunctions.resize(typeId + 1);
        }
        serializationFunctions[typeId] = func;
    }

    void Serializer::setDeserializationFunction(int typeId, const std::function<void(YAML::Node &, void *)> &func) {
        if(deserializationFunctions.size() <= typeId){
            deserializationFunctions.resize(typeId + 1);
        }
        deserializationFunctions[typeId] = func;
    }

    template<typename T>
    void defineAssetFunctions(Serializer &serializer){
        serializer.setSerializationFunction<Ref<T>>([](YAML::Emitter &out, Ref<T> &v){
            std::string file = env->assets->getFile(v);
            if(file == ""){
                out << YAML::Null;
            }else{
                out << file;
            }
        });
        serializer.setDeserializationFunction<Ref<T>>([](YAML::Node &in, Ref<T> &v){
            if(!in.IsNull()){
                v = env->assets->get<T>(in.as<std::string>(""));
            }
        });
    }

#define S_VALUE(type) setSerializationFunction<type>([](YAML::Emitter &out, type &v){out << v;});
#define S_FLOW(type, seq) setSerializationFunction<type>([](YAML::Emitter &out, type &v){out << YAML::Flow << YAML::BeginSeq << seq << YAML::EndSeq;});
#define D_VALUE(type) setDeserializationFunction<type>([](YAML::Node &in, type &v){v = in.as<type>(type());});

    void Serializer::startup() {
        S_VALUE(float)
        S_VALUE(double)
        S_VALUE(int)
        S_VALUE(uint32_t)
        S_VALUE(bool)

        S_FLOW(glm::vec2, v.x << v.y);
        S_FLOW(glm::vec3, v.x << v.y << v.z);
        S_FLOW(glm::vec4, v.x << v.y << v.z << v.w);
        S_FLOW(Color, (int)v.r << (int)v.g << (int)v.b << (int)v.a);


        D_VALUE(float)
        D_VALUE(double)
        D_VALUE(int)
        D_VALUE(uint32_t)
        D_VALUE(bool)

        setDeserializationFunction<glm::vec2>([](YAML::Node &in, glm::vec2 &v){
            v.x = in[0].as<float>(0);
            v.y = in[1].as<float>(0);
        });
        setDeserializationFunction<glm::vec3>([](YAML::Node &in, glm::vec3 &v){
            v.x = in[0].as<float>(0);
            v.y = in[1].as<float>(0);
            v.z = in[2].as<float>(0);
        });
        setDeserializationFunction<glm::vec4>([](YAML::Node &in, glm::vec4 &v){
            v.x = in[0].as<float>(0);
            v.y = in[1].as<float>(0);
            v.z = in[2].as<float>(0);
            v.w = in[3].as<float>(0);
        });
        setDeserializationFunction<Color>([](YAML::Node &in, Color &v){
            v.r = in[0].as<int>(255);
            v.g = in[1].as<int>(255);
            v.b = in[2].as<int>(255);
            v.a = in[3].as<int>(255);
        });

        defineAssetFunctions<Mesh>(*this);
        defineAssetFunctions<Texture>(*this);
        defineAssetFunctions<Material>(*this);
        defineAssetFunctions<Shader>(*this);

    }

#undef S_FLOW
#undef S_VALUE
#undef D_VALUE

}

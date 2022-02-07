//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Serializer.h"
#include "render/Color.h"
#include "render/Mesh.h"
#include "AssetManager.h"
#include "render/Material.h"
#include "ComponentCache.h"
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

        if(desc && desc->constants.size() > 0){
            int value = *(int*)data;
            for(auto &c : desc->constants){
                if(c.value == value){
                    out << YAML::Value << c.name;
                    return;
                }
            }
        }

        if(desc && desc->member.size() > 0){
            out << YAML::Value << YAML::BeginMap;
            for(auto &m : desc->member){
                if (!(m.flags & Reflection::NOT_SERIALIZED)) {
                    out << YAML::Key << m.name;
                    serializeType(out, m.type->typeId, (uint8_t*)data + m.offset);
                }
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

        if(desc && desc->constants.size() > 0){
            int &value = *(int*)data;
            std::string name = in.Scalar();
            for(auto &c : desc->constants){
                if(c.name == name){
                    value = c.value;
                    return;
                }
            }
        }

        if(desc && desc->member.size() > 0){
            for(auto &m : desc->member){
                YAML::Node type = in[m.name];
                if(type){
                    deserializeType(type, m.type->typeId, (uint8_t*)data + m.offset);
                }
            }
        }
    }

    void Serializer::serializeEntity(YAML::Emitter &out, Scene &scene, EntityId id) {
        out << YAML::BeginMap;
        out << YAML::Key << "id" << YAML::Value << id;
        for(auto &desc : env->reflection->getDescriptors()){
            if(desc && scene.hasComponent(desc->typeId, id)){
                if (desc->typeId == env->reflection->getTypeId<ComponentCache>()) {
                    auto &cache = scene.getComponent<ComponentCache>(id);
                    for (auto& i : cache.data) {
                        out << YAML::Key << i.first;
                        out << YAML::Value << YAML::Load(i.second);
                    }
                }
                else {
                    if (!(desc->flags & Reflection::NOT_SERIALIZED)) {
                        out << YAML::Key << desc->name;
                        serializeType(out, desc->typeId, scene.getComponent(desc->typeId, id));
                    }
                }
            }
        }
        out << YAML::EndMap;
    }

    void Serializer::deserializeEntity(YAML::Node &in, Scene &scene) {
        EntityId id = in["id"].as<EntityId>(-1);
        id = scene.addEntityHinted(id);
        for(auto iter : in){
            if (iter.first.Scalar() != "id") {
                auto* desc = env->reflection->getType(iter.first.Scalar());
                if (desc) {
                    void* data = scene.addComponent(desc->typeId, id);
                    deserializeType(iter.second, desc->typeId, data);
                }
                else if (env->editor) {
                    if (scene.hasComponent<ComponentCache>(id)) {
                        auto& cache = scene.getComponent<ComponentCache>(id);
                        YAML::Emitter e;
                        e << iter.second;
                        cache.cache(iter.first.Scalar(), e.c_str());
                    }
                    else {
                        auto& cache = scene.addComponent<ComponentCache>(id);
                        YAML::Emitter e;
                        e << iter.second;
                        cache.cache(iter.first.Scalar(), e.c_str());
                    }
                }
            }
        }
    }

    void Serializer::serializeScene(YAML::Emitter &out, Scene &scene) {
        out << YAML::BeginSeq;
        scene.view<>().each([&](EntityId id){
            serializeEntity(out, scene, id);
        });
        out << YAML::EndSeq;
    }

    void Serializer::deserializeScene(YAML::Node &in, Scene &scene) {
        for(auto entity : in){
            deserializeEntity(entity, scene);
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

    void Serializer::unsetSerializationFunction(int typeId) {
        if (serializationFunctions.size() > typeId) {
            serializationFunctions[typeId] = nullptr;
        }
    }

    void Serializer::unsetDeserializationFunction(int typeId) {
        if (deserializationFunctions.size() > typeId) {
            deserializationFunctions[typeId] = nullptr;
        }
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

    void defineVectorFunctions(Serializer& serializer, int typeId) {
        serializer.setSerializationFunction(typeId, [&, typeId](YAML::Emitter& out, void *v) {
            auto *desc = env->reflection->getType(typeId);
            if (desc && desc->baseType) {
                if (desc->flags & Reflection::VECTOR) {
                    int size = desc->vectorSize(v);
                    out << YAML::BeginSeq;
                    for (int i = 0; i < size; i++) {
                        serializer.serializeType(out, desc->baseType->typeId, desc->vectorGet(v, i));
                    }
                    out << YAML::EndSeq;
                }
            }
        });
        serializer.setDeserializationFunction(typeId, [&, typeId](YAML::Node& in, void* v) {
            auto* desc = env->reflection->getType(typeId);
            if (desc && desc->baseType) {
                if (desc->flags & Reflection::VECTOR) {
                    int i = 0;
                    desc->vectorClear(v);
                    for (auto iter : in) {
                        desc->vectorInsert(v, i, nullptr);
                        void *ptr = desc->vectorGet(v, i);
                        i++;
                        serializer.deserializeType(iter, desc->baseType->typeId, ptr);
                    }

                }
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
        S_VALUE(std::string)

        S_FLOW(glm::vec2, v.x << v.y);
        S_FLOW(glm::vec3, v.x << v.y << v.z);
        S_FLOW(glm::vec4, v.x << v.y << v.z << v.w);
        S_FLOW(Color, (int)v.r << (int)v.g << (int)v.b << (int)v.a);


        D_VALUE(float)
        D_VALUE(double)
        D_VALUE(int)
        D_VALUE(uint32_t)
        D_VALUE(bool)
        D_VALUE(std::string)

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
        defineAssetFunctions<Prefab>(*this);

        for (auto &desc : env->reflection->getDescriptors()) {
            if (desc && (desc->flags & Reflection::VECTOR)) {
                defineVectorFunctions(*this, desc->typeId);
            }
        }
        env->signals->typeRegister.addCallback("Seriaizer", [this](int typeId) {
            for (auto& desc : env->reflection->getDescriptors()) {
                if (desc && (desc->flags & Reflection::VECTOR)) {
                    defineVectorFunctions(*this, desc->typeId);
                }
            }
        });
        env->signals->typeUnregister.addCallback("Seriaizer", [this](int typeId) {
            
        });
    }

    void Serializer::serializePrefab(YAML::Emitter &out, Prefab &prefab) {
        out << YAML::BeginMap;
        for (ComponentBuffer& comp : prefab.getComponents()) {
            auto* desc = env->reflection->getType(comp.getTypeId());
            if (desc) {
                out << YAML::Key << desc->name;
                env->serializer->serializeType(out, comp.getTypeId(), comp.get());
            }
        }

        if(prefab.getChildren().size() > 0){
            out << YAML::Key << "Children";
            out << YAML::BeginSeq;
            for(auto &child : prefab.getChildren()){
                serializePrefab(out, *child);
            }
            out << YAML::EndSeq;
        }

        out << YAML::EndMap;
    }

    void Serializer::deserializePrefab(YAML::Node &in, Prefab &prefab) {
        for(auto &desc : env->reflection->getDescriptors()){
            if(desc) {
                YAML::Node component = in[desc->name];
                if (component) {
                    void *comp = prefab.addComponent(desc->typeId);
                    env->serializer->deserializeType(component, desc->typeId, comp);
                }
            }
        }

        YAML::Node children = in["Children"];
        if(children){
            for(auto child : children){
                deserializePrefab(child, prefab.addChild());
            }
        }
    }

#undef S_FLOW
#undef S_VALUE
#undef D_VALUE

}

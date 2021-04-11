//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Serializer.h"
#include "tridot/render/Color.h"
#include "tridot/render/Light.h"
#include "tridot/render/Material.h"
#include "tridot/render/Texture.h"
#include "tridot/render/Mesh.h"
#include "tridot/render/Shader.h"
#include "tridot/engine/Physics.h"
#include "tridot/engine/Plugin.h"
#include "tridot/components/ComponentCache.h"
#include "tridot/components/Tag.h"
#include <fstream>

using namespace ecs;

namespace tridot {

    void Serializer::serializeType(Reflection::Type *type, const std::string &name, YAML::Emitter &out, void *ptr, ResourceManager &resources){
        out << YAML::Key << name;
        if(type->id() == Reflection::id<float>()){
            out << YAML::Value << *(float*)ptr;
        }else if(type->id() == Reflection::id<int>()){
            out << YAML::Value << *(int*)ptr;
        }else if(type->id() == Reflection::id<uint32_t>()){
            out << YAML::Value << *(uint32_t*)ptr;
        }else if(type->id() == Reflection::id<uint64_t>()){
            out << YAML::Value << *(uint64_t*)ptr;
        }else if(type->id() == Reflection::id<bool>()){
            out << YAML::Value << *(bool*)ptr;
        }else if(type->id() == Reflection::id<std::string>()){
            std::string &v = *(std::string*)ptr;
            out << YAML::Value << v;
        }else if(type->id() == Reflection::id<Color>()){
            Color &v = *(Color*)ptr;
            out << YAML::Flow << YAML::BeginSeq << v.vec().x << v.vec().y << v.vec().z << v.vec().w << YAML::EndSeq;
        }else if(type->id() == Reflection::id<glm::vec2>()){
            glm::vec2 &v = *(glm::vec2*)ptr;
            out << YAML::Flow << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
        }else if(type->id() == Reflection::id<glm::vec3>()){
            glm::vec3 &v = *(glm::vec3*)ptr;
            out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        }else if(type->id() == Reflection::id<glm::vec4>()){
            glm::vec4 &v = *(glm::vec4*)ptr;
            out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
        }else if(type->id() == Reflection::id<Tag>()){
            Tag &v = *(Tag*)ptr;
            out << v.tag;
        }else if(type->id() == Reflection::id<uuid>()){
            uuid &v = *(uuid*)ptr;
            out << v.str();
        }else if(type->id() == Reflection::id<LightType>()){
            out << YAML::Value << *(int*)ptr;
        }else if(type->id() == Reflection::id<Material::Mapping>()){
            out << YAML::Value << *(int*)ptr;
        }else if(type->id() == Reflection::id<Collider::Type>()){
            out << YAML::Value << *(int*)ptr;
        }else if(type->id() == Reflection::id<Ref<Texture>>()){
            Ref<Texture> &v = *(Ref<Texture>*)ptr;
            out << YAML::Value << resources.getName(v);
        }else if(type->id() == Reflection::id<Ref<Material>>()){
            Ref<Material> &v = *(Ref<Material>*)ptr;
            out << YAML::Value << resources.getName(v);
        }else if(type->id() == Reflection::id<Ref<Mesh>>()){
            Ref<Mesh> &v = *(Ref<Mesh>*)ptr;
            out << YAML::Value << resources.getName(v);
        }else if(type->id() == Reflection::id<Ref<Shader>>()){
            Ref<Shader> &v = *(Ref<Shader>*)ptr;
            out << YAML::Value << resources.getName(v);
        }else if(type->id() == Reflection::id<YAML::Node>()){
            YAML::Node &v = *(YAML::Node*)ptr;
            out << v;
        }else {
            if(type->member().size() == 0){
                out << YAML::Value << YAML::Null;
            }else{
                out << YAML::BeginMap;
                for(auto &member : type->member()){
                    serializeType(&Reflection::get(member.typeId), member.name, out, (char*)ptr + member.offset, resources);
                }
                out << YAML::EndMap;
            }
        }
    }

    void Serializer::deserializeType(Reflection::Type *type, YAML::Node &in, void *ptr, ResourceManager &resources) {
        if(type->id() == Reflection::id<float>()){
            *(float*)ptr = in.as<float>(0);
        }else if(type->id() == Reflection::id<int>()){
            *(int*)ptr = in.as<int>(0);
        }else if(type->id() == Reflection::id<uint32_t>()){
            *(uint32_t*)ptr = in.as<uint32_t>(0);
        }else if(type->id() == Reflection::id<uint64_t>()){
            *(uint64_t*)ptr = in.as<uint64_t>(0);
        }else if(type->id() == Reflection::id<bool>()){
            *(bool*)ptr = in.as<bool>(false);
        }else if(type->id() == Reflection::id<std::string>()){
            *(std::string*)ptr = in.as<std::string>("");
        }else if(type->id() == Reflection::id<Color>()){
            glm::vec4 v;
            for(int i = 0; i < in.size(); i++){
                if(i <= 4){
                    *((float*)&v + i) = in[i].as<float>(0);
                }
            }
            *(Color*)ptr = Color(v);
        }else if(type->id() == Reflection::id<glm::vec2>()){
            for(int i = 0; i < in.size(); i++){
                if(i <= 2){
                    *((float*)ptr + i) = in[i].as<float>(0);
                }
            }
        }else if(type->id() == Reflection::id<glm::vec3>()){
            for(int i = 0; i < in.size(); i++){
                if(i <= 3){
                    *((float*)ptr + i) = in[i].as<float>(0);
                }
            }
        }else if(type->id() == Reflection::id<glm::vec4>()){
            for(int i = 0; i < in.size(); i++){
                if(i <= 4){
                    *((float*)ptr + i) = in[i].as<float>(0);
                }
            }
        }else if(type->id() == Reflection::id<uuid>()){
            (*(uuid*)ptr).set(in.as<std::string>(""));
        }else if(type->id() == Reflection::id<Tag>()){
            (*(Tag*)ptr).tag = in.as<std::string>("");
        }else if(type->id() == Reflection::id<LightType>()){
            *(int*)ptr = in.as<int>(0);
        }else if(type->id() == Reflection::id<Material::Mapping>()){
            *(int*)ptr = in.as<int>(0);
        }else if(type->id() == Reflection::id<Collider::Type>()){
            *(int*)ptr = in.as<int>(0);
        }else if(type->id() == Reflection::id<Ref<Texture>>()){
            std::string name = in.as<std::string>("");
            if(!name.empty()){
                Ref<Texture> &v = *(Ref<Texture>*)ptr;
                v = resources.get<Texture>(name);
            }
        }else if(type->id() == Reflection::id<Ref<Material>>()){
            std::string name = in.as<std::string>("");
            if(!name.empty()) {
                Ref<Material> &v = *(Ref<Material> *) ptr;
                v = resources.get<Material>(name);
            }
        }else if(type->id() == Reflection::id<Ref<Mesh>>()){
            std::string name = in.as<std::string>("");
            if(!name.empty()) {
                Ref<Mesh> &v = *(Ref<Mesh> *) ptr;
                v = resources.get<Mesh>(name);
            }
        }else if(type->id() == Reflection::id<Ref<Mesh>>()){
            std::string name = in.as<std::string>("");
            if(!name.empty()) {
                Ref<Shader> &v = *(Ref<Shader> *) ptr;
                v = resources.get<Shader>(name);
            }
        }else if(type->id() == Reflection::id<YAML::Node>()){
            YAML::Node &v = *(YAML::Node*)ptr;
            v = in.as<YAML::Node>();
        }else {
            for(auto &member : type->member()){
                auto node = in[member.name];
                if(node){
                    deserializeType(&Reflection::get(member.typeId), node, (char*)ptr + member.offset, resources);
                }
            }
        }
    }

    bool Serializer::save(const std::string &file, ecs::Registry &reg, ResourceManager &resources, bool binary) {
        if(binary){
            Log::error(false, "binary serialisation is not yet implemented");
            return false;
        }else{
            YAML::Emitter out;
            out << YAML::BeginMap;

            //Entities
            {
                out << YAML::Key << "Entities" << YAML::BeginSeq;

                for (EntityId id : reg.getEntityPool().getEntities()) {
                    out << YAML::BeginMap;
                    out << YAML::Key << "id" << YAML::Value << id;

                    for (auto &type : Reflection::getTypes()) {
                        if(type) {
                            auto *pool = reg.getPool(type->id());
                            if (pool && pool->has(id)) {
                                void *ptr = pool->getById(id);
                                serializeType(type, type->name(), out, ptr, resources);
                            }
                        }
                    }

                    out << YAML::EndMap;
                }

                out << YAML::EndSeq;
            }

            //Materials
            {
                out << YAML::Key << "Materials" << YAML::BeginSeq;

                auto materialNames = resources.getNames<Material>();
                for(auto &name : materialNames){
                    out << YAML::BeginMap;
                    Ref<Material> material = resources.get<Material>(name);
                    out << YAML::Key << "name" << YAML::Value << name;
                    serializeType(&Reflection::get<Material>(), "material", out, material.get(), resources);
                    out << YAML::EndMap;
                }

                out << YAML::EndSeq;
            }

            //Plugins
            {
                auto pluginNames = resources.getNames<Plugin>();
                if(pluginNames.size() > 0) {
                    out << YAML::Key << "Plugins" << YAML::BeginSeq;

                    for (auto &name : pluginNames) {
                        out << YAML::BeginMap;
                        Ref<Plugin> plugin = resources.get<Plugin>(name);
                        out << YAML::Key << "name" << YAML::Value << name;
                        out << YAML::EndMap;

                    }

                    out << YAML::EndSeq;
                }
            }

            out << YAML::EndMap;
            std::ofstream stream(file);
            stream << out.c_str();
            return true;
        }
    }

    bool Serializer::load(const std::string &file, ecs::Registry &reg, ResourceManager &resources) {
        std::ifstream stream(file);
        if(stream.is_open()){
            YAML::Node data;
            try{
                data = YAML::Load(stream);
            }catch (...){
                return false;
            }

            //Materials
            if(auto materials = data["Materials"]){
                for(auto material : materials){
                    std::string name = material["name"].as<std::string>("");
                    if(!name.empty()){
                        auto node = material["material"];
                        if(node){
                            Ref<Material> &mat = resources.set<Material>(name);
                            deserializeType(&Reflection::get<Material>(), node, mat.get(), resources);
                        }
                    }
                }
            }

            //Plugins
            if(auto plugins = data["Plugins"]){
                for(auto plugin : plugins){
                    std::string name = plugin["name"].as<std::string>("");
                    if(!name.empty()){
                        resources.get<Plugin>(name);
                    }
                }
            }

            //Entities
            if(auto entities = data["Entities"]){
                reg.clear();
                for(auto entity : entities){
                    EntityId id = entity["id"].as<EntityId>(-1);
                    id = reg.createHinted(id);
                    entity.remove("id");

                    for(auto &type : Reflection::getTypes()){
                        if(type) {
                            auto comp = entity[type->name()];
                            if (comp) {
                                auto *pool = reg.getPool(type->id());
                                if (pool) {
                                    uint32_t index = pool->add(id, nullptr);
                                    void *ptr = pool->get(index);
                                    deserializeType(type, comp, ptr, resources);
                                    entity.remove(type->name());
                                } else {
                                    Log::warning("no component pool present for ", type->name());
                                }
                                if(reg.has<ComponentCache>(id)){
                                    reg.get<ComponentCache>(id).update(id);
                                }
                            }
                        }
                    }

                    if(entity.size() > 0){
                        auto &cache = reg.add<ComponentCache>(id);
                        for(auto comp : entity){
                            if(comp.first){
                                cache.data[comp.first] = comp.second;
                            }
                        }
                    }
                }
            }else{
                return false;
            }
            return true;
        }
        return false;
    }

}
//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Material.h"
#include "core/core.h"
#include "engine/Serializer.h"
#include "engine/AssetManager.h"

namespace tri {

    Material::Material(Color color, const Ref<Texture> &texture, Material::Mapping mapping) {
        this->color = color;
        this->mapping = mapping;
        roughness = 1.0;
        metallic = 0.0;
        normalMapFactor = 1.0;
        emissive = 0;

        this->texture = texture;
        normalMap = nullptr;
        roughnessMap = nullptr;
        metallicMap = nullptr;
        ambientOcclusionMap = nullptr;
        displacementMap = nullptr;

        offset = {0, 0};
        scale = {1, 1};

        textureOffset = {0, 0};
        textureScale = {1, 1};

        normalMapOffset = {0, 0};
        normalMapScale = {1, 1};

        roughnessMapOffset = {0, 0};
        roughnessMapScale = {1, 1};

        metallicMapOffset = {0, 0};
        metallicMapScale = {1, 1};

        ambientOcclusionMapOffset = {0, 0};
        ambientOcclusionMapScale = {1, 1};

        displacementMapOffset = {0, 0};
        displacementMapScale = {1, 1};

        shader = nullptr;
    }

    bool Material::isOpaque() {
        return color.a == 255;
    }

    bool Material::load(const std::string &file) {
        SerialData data;
        bool valid = env->serializer->loadFromFile(data, file);
        env->serializer->deserializeClass(Reflection::getClassId<Material>(), this, data);
        if (valid) {
            env->console->trace("loaded material %s", file.c_str());
        }
        return valid;
    }

    bool Material::save(const std::string &file) {
        env->assetManager->setOptions(file, AssetManager::Options::NO_RELOAD_ONCE);
        SerialData data;
        std::ofstream stream(file);
        data.emitter = std::make_shared<YAML::Emitter>(stream);
        env->serializer->serializeClass(Reflection::getClassId<Material>(), this, data);
        return true;
    }

    TRI_CLASS(Material::Mapping);
    TRI_ENUM3(Material::Mapping, UV, TRI_PLANAR, SCALE_TRI_PLANAR);

    TRI_ASSET(Material);
    TRI_PROPERTY(Material, color);
    TRI_PROPERTY(Material, mapping);
    TRI_PROPERTIES4(Material, roughness, metallic, normalMapFactor, emissive);
    TRI_PROPERTY_RANGE(Material, roughness, 0, 1);
    TRI_PROPERTY_RANGE(Material, metallic, 0, 1);
    TRI_PROPERTY_RANGE(Material, normalMapFactor, 0, 1);
    TRI_PROPERTY_RANGE(Material, emissive, 0, 1);
    TRI_PROPERTY(Material, texture);
    TRI_PROPERTY(Material, normalMap);
    TRI_PROPERTY(Material, roughnessMap);
    TRI_PROPERTY(Material, metallicMap);
    TRI_PROPERTY(Material, ambientOcclusionMap);
    TRI_PROPERTY(Material, displacementMap);
    TRI_PROPERTY(Material, shader);
    TRI_PROPERTY(Material, offset);
    TRI_PROPERTY(Material, scale);
    TRI_PROPERTY(Material, textureOffset);
    TRI_PROPERTY(Material, textureScale);
    TRI_PROPERTY(Material, normalMapOffset);
    TRI_PROPERTY(Material, normalMapScale);
    TRI_PROPERTY(Material, roughnessMapOffset);
    TRI_PROPERTY(Material, roughnessMapScale);
    TRI_PROPERTY(Material, metallicMapOffset);
    TRI_PROPERTY(Material, metallicMapScale);
    TRI_PROPERTY(Material, ambientOcclusionMapOffset);
    TRI_PROPERTY(Material, ambientOcclusionMapScale);
    TRI_PROPERTY(Material, displacementMapOffset);
    TRI_PROPERTY(Material, displacementMapScale);

}
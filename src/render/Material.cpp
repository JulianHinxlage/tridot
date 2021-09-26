//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Material.h"
#include "core/core.h"
#include "engine/Serializer.h"

namespace tri {

    Material::Material(Color color, const Ref<Texture> &texture, Material::Mapping mapping) {
        this->color = color;
        this->mapping = mapping;
        roughness = 1.0;
        metallic = 0.0;
        normalMapFactor = 1.0;

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
        return env->serializer->deserializeType(file, env->reflection->getTypeId<Material>(), this);
    }

    bool Material::save(const std::string &file) {
        return env->serializer->serializeType(file, env->reflection->getTypeId<Material>(), this);
    }

    TRI_REGISTER_TYPE(Material::Mapping);
    TRI_REGISTER_CONSTANT(Material::Mapping, UV);
    TRI_REGISTER_CONSTANT(Material::Mapping, TRI_PLANAR);
    TRI_REGISTER_CONSTANT(Material::Mapping, SCALE_TRI_PLANAR);

    TRI_REGISTER_COMPONENT(Material);
    TRI_REGISTER_MEMBER(Material, color);
    TRI_REGISTER_MEMBER(Material, mapping);
    TRI_REGISTER_MEMBER_RANGE(Material, roughness, 0, 1);
    TRI_REGISTER_MEMBER_RANGE(Material, metallic, 0, 1);
    TRI_REGISTER_MEMBER_RANGE(Material, normalMapFactor, 0, 1);
    TRI_REGISTER_MEMBER(Material, texture);
    TRI_REGISTER_MEMBER(Material, normalMap);
    TRI_REGISTER_MEMBER(Material, roughnessMap);
    TRI_REGISTER_MEMBER(Material, metallicMap);
    TRI_REGISTER_MEMBER(Material, ambientOcclusionMap);
    TRI_REGISTER_MEMBER(Material, displacementMap);
    TRI_REGISTER_MEMBER(Material, shader);
    TRI_REGISTER_MEMBER(Material, offset);
    TRI_REGISTER_MEMBER(Material, scale);
    TRI_REGISTER_MEMBER(Material, textureOffset);
    TRI_REGISTER_MEMBER(Material, textureScale);
    TRI_REGISTER_MEMBER(Material, normalMapOffset);
    TRI_REGISTER_MEMBER(Material, normalMapScale);
    TRI_REGISTER_MEMBER(Material, roughnessMapOffset);
    TRI_REGISTER_MEMBER(Material, roughnessMapScale);
    TRI_REGISTER_MEMBER(Material, metallicMapOffset);
    TRI_REGISTER_MEMBER(Material, metallicMapScale);
    TRI_REGISTER_MEMBER(Material, ambientOcclusionMapOffset);
    TRI_REGISTER_MEMBER(Material, ambientOcclusionMapScale);
    TRI_REGISTER_MEMBER(Material, displacementMapOffset);
    TRI_REGISTER_MEMBER(Material, displacementMapScale);

}
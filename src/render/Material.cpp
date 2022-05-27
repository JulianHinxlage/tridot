//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Material.h"
#include "core/core.h"
//#include "engine/Serializer.h"

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
        //return env->serializer->deserializeType(file, env->reflection->getTypeId<Material>(), this);
        return false;
    }

    bool Material::save(const std::string &file) {
        //if(env->serializer->serializeType(file, env->reflection->getTypeId<Material>(), this)){
        //    env->console->debug("saved material ", file);
        //    return true;
        //}else{
        //    return false;
        //}
        return false;
    }

    TRI_CLASS(Material::Mapping);
    TRI_ENUM3(Material::Mapping, UV, TRI_PLANAR, SCALE_TRI_PLANAR);

    TRI_CLASS(Material);
    //TRI_MEMBER(Material, color);
    //TRI_MEMBER(Material, mapping);
    //TRI_MEMBER_RANGE(Material, roughness, 0, 1);
    //TRI_MEMBER_RANGE(Material, metallic, 0, 1);
    //TRI_MEMBER_RANGE(Material, normalMapFactor, 0, 1);
    //TRI_MEMBER_RANGE(Material, emissive, 0, 1);
    //TRI_MEMBER(Material, texture);
    //TRI_MEMBER(Material, normalMap);
    //TRI_MEMBER(Material, roughnessMap);
    //TRI_MEMBER(Material, metallicMap);
    //TRI_MEMBER(Material, ambientOcclusionMap);
    //TRI_MEMBER(Material, displacementMap);
    //TRI_MEMBER(Material, shader);
    //TRI_MEMBER(Material, offset);
    //TRI_MEMBER(Material, scale);
    //TRI_MEMBER(Material, textureOffset);
    //TRI_MEMBER(Material, textureScale);
    //TRI_MEMBER(Material, normalMapOffset);
    //TRI_MEMBER(Material, normalMapScale);
    //TRI_MEMBER(Material, roughnessMapOffset);
    //TRI_MEMBER(Material, roughnessMapScale);
    //TRI_MEMBER(Material, metallicMapOffset);
    //TRI_MEMBER(Material, metallicMapScale);
    //TRI_MEMBER(Material, ambientOcclusionMapOffset);
    //TRI_MEMBER(Material, ambientOcclusionMapScale);
    //TRI_MEMBER(Material, displacementMapOffset);
    //TRI_MEMBER(Material, displacementMapScale);

}
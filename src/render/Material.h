//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Color.h"
#include "Texture.h"
#include "Shader.h"
#include "engine/Asset.h"
#include "core/util/Ref.h"

namespace tri {

    class Material : public Asset {
    public:
        enum Mapping{
            UV = 0,
            TRI_PLANAR = 1,
            SCALE_TRI_PLANAR = 2,
        };

        Color color;
        Mapping mapping;
        float roughness;
        float metallic;
        float normalMapFactor;

        Ref<Texture> texture;
        Ref<Texture> normalMap;
        Ref<Texture> roughnessMap;
        Ref<Texture> metallicMap;
        Ref<Texture> ambientOcclusionMap;
        Ref<Texture> displacementMap;

        glm::vec2 offset;
        glm::vec2 scale;

        glm::vec2 textureOffset;
        glm::vec2 textureScale;

        glm::vec2 normalMapOffset;
        glm::vec2 normalMapScale;

        glm::vec2 roughnessMapOffset;
        glm::vec2 roughnessMapScale;

        glm::vec2 metallicMapOffset;
        glm::vec2 metallicMapScale;

        glm::vec2 ambientOcclusionMapOffset;
        glm::vec2 ambientOcclusionMapScale;

        glm::vec2 displacementMapOffset;
        glm::vec2 displacementMapScale;

        Ref<Shader> shader;

        Material(Color color = Color::white, const Ref<Texture> &texture = nullptr, Mapping mapping = UV);

        bool isOpaque();

        bool load(const std::string &file) override;
        bool save(const std::string &file) override;
    };

}


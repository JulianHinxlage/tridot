//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Color.h"
#include "Texture.h"
#include "Shader.h"
#include "tridot/util/Ref.h"

namespace tridot {

    class Material {
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

        glm::vec2 textureOffset;
        glm::vec2 textureScale;

        glm::vec2 normalMapOffset;
        glm::vec2 normalMapScale;

        glm::vec2 roughnessMapOffset;
        glm::vec2 roughnessMapScale;

        glm::vec2 metallicMapOffset;
        glm::vec2 metallicMapScale;

        Ref<Shader> shader;

        Material(Color color = Color::white, const Ref<Texture> &texture = nullptr, Mapping mapping = UV);
    };

}


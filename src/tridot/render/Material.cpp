//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Material.h"

namespace tridot {

    Material::Material(Color color, const Ref<Texture> &texture, Material::Mapping mapping) {
        this->color = color;
        this->mapping = mapping;
        roughness = 0.6;
        metallic = 0.0;
        normalMapFactor = 0.5;

        this->texture = texture;
        normalMap = nullptr;
        roughnessMap = nullptr;
        metallicMap = nullptr;

        textureOffset = {0, 0};
        textureScale = {1, 1};

        normalMapOffset = {0, 0};
        normalMapScale = {1, 1};

        roughnessMapOffset = {0, 0};
        roughnessMapScale = {1, 1};

        metallicMapOffset = {0, 0};
        metallicMapScale = {1, 1};

        shader = nullptr;
    }

}
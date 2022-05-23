//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "render/Material.h"
#include "render/Mesh.h"

namespace tri {

    class Skybox {
    public:
        Ref<Texture> texture;
        bool useSky;
        bool useIBL;
        Color color;
        float intensity;
        Ref<Texture> irradianceMap;

        Skybox(Ref<Texture> texture = nullptr, bool useSky = true, bool useIBL = false, Color color = Color::white, float intesity = 1.0f)
            : texture(texture), useSky(useSky), useIBL(useIBL), color(color), intensity(intesity), irradianceMap(nullptr) {}
    };

}
